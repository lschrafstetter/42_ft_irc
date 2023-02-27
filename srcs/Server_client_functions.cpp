#include "Server.hpp"

namespace irc {

/**
 * @brief The PASS command is used to set a 'connection password'.  The
   password can and must be set before any attempt to register the
   connection is made.  Currently this requires that clients send a PASS
   command before sending the NICK/USER combination and servers *must*
   send a PASS command before any SERVER command.
 *
 * @param fd the client's file descriptor
 * @param message message[0] = "PASS", message[1] = <password>
 */
void Server::pass_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  if (client.get_status(PASS_AUTH) == true) {
    // Error 462: You may not reregister
    queue_.push(
        std::make_pair(fd, numeric_reply_(462, fd, client.get_nickname())));
    return;
  }
  if (message.size() == 1) {
    // Error 461: Not enough parameters
    queue_.push(
        std::make_pair(fd, numeric_reply_(461, fd, client.get_nickname())));
    return;
  }
  if (message.size() == 2 && message[1] == password_) {
#if DEBUG
    std::cout << "Password accepted; access permitted\n";
#endif
    client.set_status(PASS_AUTH);
    if (client.is_authorized())
      welcome_(fd);
  } else {
    // Error 464: password incorrect
    queue_.push(std::make_pair(fd, numeric_reply_(464, fd, message[1])));
  }
}

bool Server::search_nick_list_(const std::string &nick) const {
  std::map<int, Client>::const_iterator it;
  for (it = clients_.begin(); it != clients_.end(); ++it) {
    if (irc_stringissame(nick, (*it).second.get_nickname())) {
      return 1;
    }
  }
  return 0;
}

int Server::search_user_list_(const std::string &user) const {
  std::map<int, Client>::const_iterator it;
  for (it = clients_.begin(); it != clients_.end(); ++it) {
    if (irc_stringissame(user, (*it).second.get_username())) {
      return (*it).first;
    }
  }
  return -1;
}

/**
 * @brief The USER message is used at the beginning of connection to specify
   the username, hostname, servername and realname of s new user.  It is
   also used in communication between servers to indicate new user
   arriving on IRC, since only after both USER and NICK have been
   received from a client does a user become registered.

 *
 * @param fd the client's fd
 * @param message message[0] = "USER", message[1] = <username>, message[2] =
 <hostname>, message[3] = <servername>, message[4] = <realname>
 */
void Server::user_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  if (!client.get_status(PASS_AUTH)) {
    // Error 464: Password incorrect
    queue_.push(std::make_pair(fd, numeric_reply_(464, fd, "Enter password")));
    return;
  }
  if (client.get_status(USER_AUTH)) {
    // Error 462: You may not reregister << Comment from Lukas: Is this the
    // right parameter to hand the function?
    queue_.push(std::make_pair(fd, numeric_reply_(462, fd, "Enter password")));
    return;
  }
  if (message.size() < 4) {
    // Error 461: Not enough parameters << Comment from Lukas: Is this the right
    // parameter to hand the function?
    queue_.push(std::make_pair(fd, numeric_reply_(461, fd, "Enter password")));
    return;
  }
  //: server 468 nick :Your username is invalid.
  //: server 468 nick :Connect with your real username, in lowercase.
  //: server 468 nick :If your mail address were foo@bar.com, your username
  //: would be foo.
  // return ;
  clients_[fd].set_username(message[1]);
  clients_[fd].set_status(USER_AUTH);
  if (client.is_authorized())
    welcome_(fd);
}

bool Server::has_invalid_char_(std::string nick) {
  if (nick.size() < 1)
    return 1;
  if (nick.at(0) == '#' || nick.at(0) == '&' || nick.at(0) == '@')
    return 1;
  for (size_t i = 0; i < nick.size(); ++i) {
    if (nick.at(i) == ',')
      return 1;
  }
  return 0;
}

/**
 * @brief NICK message is used to give user a nickname or change the previous
   one. Sending a NICK message is part of the registration process for a client.
 *
 * @param fd the client's file descriptor
 * @param message message[0] = "NICK", message[1] = <nickname>
 */
void Server::nick_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
#if DEBUG
  std::cout << "inside nick funcion\n";
#endif
  if (!client.get_status(PASS_AUTH)) {
    // Error 464: Password incorrect
    queue_.push(
        std::make_pair(fd, numeric_reply_(464, fd, client.get_nickname())));
    return;
  }
  if (message.size() == 1) {
    // Error 461: Not enough parameters
    queue_.push(
        std::make_pair(fd, numeric_reply_(461, fd, client.get_nickname())));
    return;
  }
  if (message[1].size() > 9 || has_invalid_char_(message[1])) {
    // 432 erroneous nickname
    queue_.push(std::make_pair(fd, numeric_reply_(432, fd, message[1])));
    return;
  }
  if (search_nick_list_(message[1])) {
    // Error 433: Nickname is already in use
    queue_.push(
        std::make_pair(fd, numeric_reply_(433, fd, client.get_nickname())));
    return;
  }
  client.set_nickname(message[1]);
  map_name_fd_.insert(std::make_pair(message[1], fd));
  if (!client.get_status(NICK_AUTH)) {
    client.set_status(NICK_AUTH);
    if (client.is_authorized())
      welcome_(fd);
  }
}

/**
 * @brief After sending a PING message to a client, the server expects a PONG
 * message back. This functions compares the PONG message's argument with the
 * expected one and updates the registration or ping status accordingly
 *
 * @param fd
 * @param message
 */
void Server::pong_(int fd, std::vector<std::string> &message) {
  if (message.size() != 2)
    return;

  Client &client = clients_[fd];
  if (client.get_expected_ping_response() == message[1]) {
    if (!client.get_status(PONG_AUTH)) {
      client.set_status(PONG_AUTH);
      if (client.is_authorized())
        welcome_(fd);
    }
    client.set_pingstatus(true);
  }
}

/**
 * @brief the user leaves all channels specified in the second parameter
 *
 * @param fd the client's file descriptor
 * @param message message[0] == "PART", message[1] ==
 * "channelname[,channelname]"
 */
void Server::part_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  std::string clientname = client.get_nickname();

  if (message.size() < 2) {
    // Error 461: Not enough parameters
    queue_.push(std::make_pair(fd, numeric_reply_(461, fd, clientname)));
    return;
  }

  std::vector<std::string> channellist = split_string(message[1], ',');

  // Leave every channel on the list individually
  for (size_t i = 0; i < channellist.size(); ++i) {
    std::string &channelname = channellist[i];
    std::map<std::string, Channel,
             irc_stringmapcomparator<std::string> >::iterator it =
        channels_.find(channelname);

    // Does the channel exist?
    if (it == channels_.end()) {
      // Error 403: No such channel
      queue_.push(std::make_pair(fd, numeric_reply_(403, fd, clientname)));
      continue;
    }

    Channel &channel = (*it).second;
    const std::vector<std::string> &users_in_channel = channel.get_users();

    // Is client a member of that channel?
    if (std::find(users_in_channel.begin(), users_in_channel.end(),
                  clientname) == users_in_channel.end()) {
      // Error 442: You're not on that channel
      queue_.push(std::make_pair(fd, numeric_reply_(442, fd, channelname)));
      continue;
    }

    // If client is the last one in the channel, delete the channel
    if (channel.get_users().size() == 1) {
      channels_.erase(channelname);
      client.remove_channel_from_channellist(channelname);
      std::stringstream servermessage;
      servermessage << ":" << clientname << " PART " << channelname;
      queue_.push(std::make_pair(fd, servermessage.str()));
    } else {
      // Send PART message to every member of the channel (including client)
      for (size_t i = 0; i < users_in_channel.size(); ++i) {
        int fd_user = map_name_fd_[users_in_channel[i]];
        std::stringstream servermessage;
        servermessage << ":" << clientname << " PART " << channelname;
        queue_.push(std::make_pair(fd_user, servermessage.str()));
      }
      channel.remove_user(clientname);
    }
  }
}

/**
 * @brief Sends a welcome message (replies 001 - 005) to a client after
 * successfull registration
 *
 * @param fd the client's file descriptor
 */
void Server::welcome_(int fd) {
  std::string clientname = clients_[fd].get_nickname();

  // 001 RPL_WELCOME
  {
    std::stringstream servermessage;
    servermessage << ":" << server_name_ << " 001 " << clientname
                  << " :Welcome to ircserv, " << clientname;
    queue_.push(std::make_pair(fd, servermessage.str()));
  }

  // 002 RPL_YOURHOST
  {
    std::stringstream servermessage;
    servermessage << ":" << server_name_ << " 002 " << clientname
                  << " :Your host is " << server_name_
                  << ", running on version 1.0";
    queue_.push(std::make_pair(fd, servermessage.str()));
  }

  // 003 RPL_CREATED
  {
    std::stringstream servermessage;
    char timebuffer[80];
    std::memset(&timebuffer, 0, sizeof(timebuffer));
    std::strftime(timebuffer, sizeof(timebuffer), "%a %b %d %Y at %H:%M:%S %Z",
                  std::localtime(&creation_time_));
    servermessage << ":" << server_name_ << " 003 " << clientname
                  << " :This server was created " << timebuffer;
    queue_.push(std::make_pair(fd, servermessage.str()));
  }

  // 004 RPL_MYINFO
  {
    std::stringstream servermessage;
    servermessage << ":" << server_name_ << " 004 " << clientname
                  << " ircserv 1.0 so oitnmlbvk olbvk";
    queue_.push(std::make_pair(fd, servermessage.str()));
  }

  // 005 RPL_ISUPPORT
  {
    std::stringstream servermessage;
    servermessage << ":" << server_name_ << " 005 " << clientname
                  << " MAXCHANNELS=10 NICKLEN=9 :MAXCHANNELS=10 NICKLEN=9 are "
                     "supported by this server";
    queue_.push(std::make_pair(fd, servermessage.str()));
  }

  // Empty helper vector
  std::vector<std::string> vec;

  // LUSER message
  lusers_(fd, vec);

  // MOTD message
  motd_(fd, vec);
}

/**
 * @brief Sends information about the server to the client. Can be called by the
 * welcome_ function after registering or by a LUSERS command
 *
 * @param fd the clients file descriptor
 * @param message doesn't matter. message[0] is "LUSERS" if the function is
 * called by a LUSERS command called after parsing
 */
void Server::lusers_(int fd, std::vector<std::string> &message) {
  (void)message;

  // 251 RPL_LUSERCLIENT (mandatory)
  // 252 RPL_LUSEROP (only if non-zero)
  // 253 RPL_LUSERUNKNOWN (only if non-zero)
  lusers_client_op_unknown_(fd);

  // 254 RPL_LUSERCHANNELS (only if non-zero)
  lusers_channels_(fd);

  // 255 RPL_LUSERME (mandatory)
  lusers_me_(fd);
}

void Server::lusers_client_op_unknown_(int fd) {
  int n_users_invis = 0;
  int n_users_non_invis = 0;
  int n_operators = 0;
  int n_unauthorized = 0;

  std::map<int, Client>::iterator it = clients_.begin();
  std::map<int, Client>::iterator end = clients_.end();

  while (it != end) {
    /* if ((*it).second.is_invisible())
      ++n_users_invis;
    else */
    ++n_users_non_invis;

    // if ((*it).second.is_operator()) ++n_operators;

    if (!(*it).second.is_authorized())
      ++n_unauthorized;

    ++it;
  }

  // 251 RPL_LUSERCLIENT (mandatory)
  {
    std::stringstream servermessage;
    servermessage << ":" << server_name_ << " 251 "
                  << clients_[fd].get_nickname() << " :There are "
                  << n_users_non_invis << " users and " << n_users_invis
                  << " invisible on 1 servers";
    queue_.push(std::make_pair(fd, servermessage.str()));
  }
  // 252 RPL_LUSEROP (only if non-zero)
  if (n_operators) {
    std::stringstream servermessage;
    servermessage << ":" << server_name_ << " 252 "
                  << clients_[fd].get_nickname() << " " << n_operators
                  << " :operator(s) online";
    queue_.push(std::make_pair(fd, servermessage.str()));
  }

  // 253 RPL_LUSERUNKNOWN (only if non-zero)
  if (n_unauthorized) {
    std::stringstream servermessage;
    servermessage.str(std::string());
    servermessage << ":" << server_name_ << " 253 "
                  << clients_[fd].get_nickname() << " " << n_unauthorized
                  << " :unknown connection(s)";
    queue_.push(std::make_pair(fd, servermessage.str()));
  }
}

void Server::lusers_channels_(int fd) {
  int n_channels = channels_.size();

  if (n_channels) {
    std::stringstream servermessage;
    servermessage << ":" << server_name_ << " 254 "
                  << clients_[fd].get_nickname() << n_channels
                  << " :channels formed";
    queue_.push(std::make_pair(fd, servermessage.str()));
  }
}

void Server::lusers_me_(int fd) {
  std::stringstream servermessage;
  servermessage << ":" << server_name_ << " 255 " << clients_[fd].get_nickname()
                << " :I have " << clients_.size() << " clients and 1 servers";
  queue_.push(std::make_pair(fd, servermessage.str()));
}

/**
 * @brief sends the message of the day to a client, consisting of a start, a
 * message and and end message. The message is contained in a separate file
 * called "motd.txt"
 *
 * @param fd the client's file descriptor
 * @param message the command which was parsed, not the message itself.
 */
void Server::motd_(int fd, std::vector<std::string> &message) {
  if (message.size() > 1 && message[1].compare(server_name_) != 0) {
    // Error 402: No such server
    queue_.push(std::make_pair(fd, numeric_reply_(402, fd, message[1])));
    return;
  }
  // RPL_MOTDSTART (375)
  motd_start_(fd);
  // RPL_MOTD (372)

  motd_message_(fd);
  // RPL_ENDOFMOTD (376)

  motd_end_(fd);
}

void Server::motd_start_(int fd) {
  std::stringstream servermessage;
  servermessage << ":" << server_name_ << " 375 " << clients_[fd].get_nickname()
                << " :- " << server_name_ << " Message of the day - ";
  queue_.push(std::make_pair(fd, servermessage.str()));
}

void Server::motd_message_(int fd) {
  std::ifstream infile;
  try {
    std::cout << "Trying to open file" << std::endl;
    infile.open("ressources/motd.txt",
                std::ifstream::in | std::ifstream::binary);
    if (infile.fail())
      throw std::exception();
  } catch (std::exception &e) {
    // Error 422: MOTD File is missing
    queue_.push(std::make_pair(fd, numeric_reply_(422, fd, "")));
    return;
  }

  std::string line;
  std::string clientname = clients_[fd].get_nickname();
  while (infile.good()) {
    std::getline(infile, line);
    std::stringstream servermessage;
    servermessage << ":" << server_name_ << " 372 " << clientname << " :"
                  << line;
    queue_.push(std::make_pair(fd, servermessage.str()));
  }
  infile.close();
}

void Server::motd_end_(int fd) {
  std::stringstream servermessage;
  servermessage << ":" << server_name_ << " 376 " << clients_[fd].get_nickname()
                << " :End of /MOTD command.";
  queue_.push(std::make_pair(fd, servermessage.str()));
}

void Server::RPL_join(const Channel &channel, const std::string &client_nick) {
  const std::string &channel_name = channel.get_channelname();
  std::stringstream servermessage;
  servermessage << ":" << client_nick << " JOIN " << channel_name;
  send_message_to_channel_(channel, servermessage.str());
}

void Server::RPL_TOPIC(const Channel &channel, const std::string &client_nick,
                       int fd) {
  const std::string &topic = channel.get_topic_name();
  const std::string &channel_name = channel.get_channelname();
  std::stringstream servermessage;
  servermessage << ":" << server_name_ << " 332 " << client_nick << " "
                << channel_name << " :" << topic;
  queue_.push(std::make_pair(fd, servermessage.str()));
}

void Server::RPL_NOTOPIC(const std::string &client_nick,
                         const std::string &channel_name, int fd) {
  std::stringstream servermessage;
  servermessage << ":" << server_name_ << " 331 " << client_nick << " " << channel_name << " :No topic is set";
  queue_.push(std::make_pair(fd, servermessage.str()));
}

void Server::RPL_NAMREPLY(const Channel &channel,
                          const std::string &client_nick, int fd) {
  const std::vector<std::string> &user_list = channel.get_users();
  const std::set<std::string, irc_stringmapcomparator<std::string> > &op_list =
      channel.get_operators();
  const std::string &channel_name = channel.get_channelname();
  std::stringstream servermessage;
  servermessage << ":" << server_name_ << " 353 " << client_nick << " = "
                << channel_name << " :";
  for (size_t i = 0; i < user_list.size(); ++i) {
    const std::string &name = user_list[i];
    if (op_list.find(name) != op_list.end())
      servermessage << "@";
    servermessage << name << " ";
  }
  queue_.push(std::make_pair(fd, servermessage.str()));
}

void Server::RPL_ENDOFNAMES(const std::string &client_nick,
                            const std::string &channel_name, int fd) {
  std::stringstream servermessage;
  servermessage << ":" << server_name_ << " 366 " << client_nick << " "
                << channel_name << " :End of /NAMES List";
  queue_.push(std::make_pair(fd, servermessage.str()));
}

void Server::check_priviliges(int fd, Client &client, Channel &channel,
                              const std::vector<std::string> &channel_key,
                              size_t *key_index) {
  const std::string &client_nick = client.get_nickname();
  const std::string &channel_name = channel.get_channelname();
  size_t key_size = channel_key.size();
  if (channel.is_user(client_nick)) // user is already in channel
    return;
  if (channel.checkflag(C_INVITE) &&
      !(channel.is_invited(client_nick))) // user is not invited
    // Error 473 :Cannot join channel (+i)
    queue_.push(std::make_pair(fd, numeric_reply_(473, fd, "")));
  else if (channel.is_banned(
               client.get_nickname(), client.get_username(),
               client.get_hostname()))  //  user is banned from channel
    // Error 474 :Cannot join channel (+b)
    queue_.push(std::make_pair(fd, numeric_reply_(474, fd, channel_name)));
  else if (!channel.get_channel_password().empty() && *key_index < key_size &&
           channel.get_channel_password() !=
               channel_key[(*key_index)++]) // key_index incrementation test!!!
                                            // // incorrect password
    // Error 475 :Cannot join channel (+k)
    queue_.push(std::make_pair(fd, numeric_reply_(475, fd, channel_name)));
  else if (channel.get_users().size() >=
           channel.get_user_limit()) //  channel userlimit exceeded
    // Error 471 :Cannot join channel (+l)
    queue_.push(std::make_pair(fd, numeric_reply_(471, fd, channel_name)));
  else if (client.get_channels_list().size() >=
           MAX_CHANNELS) //  user is in too many channels
    // Error 405 :You have joined too many channels
    queue_.push(std::make_pair(fd, numeric_reply_(405, fd, channel_name)));
  else {
    // adding user to existing channel
    channel.add_user(client_nick);
    client.add_channel(channel_name);
    RPL_join(channel, client_nick);
    if (channel.is_topic_set())
      RPL_TOPIC(channel, client_nick, fd);
    else
      RPL_NOTOPIC(client_nick, channel_name, fd);
    RPL_NAMREPLY(channel, client_nick, fd);
    RPL_ENDOFNAMES(client_nick, channel_name, fd);
  }
}

void Server::join_(int fd, std::vector<std::string> &message) {
  if (message.size() < 2) {
    // Error 461 :Not enough parameters
    queue_.push(std::make_pair(fd, numeric_reply_(461, fd, "")));
    return;
  }

  Client &client = clients_[fd];
  const std::string &client_nick = client.get_nickname();
  std::vector<std::string> channel_names = split_string(message[1], ',');
  std::vector<std::string> channel_key = split_string(message[2], ',');
  size_t key_index = 0;

  for (size_t name_index = 0; name_index < channel_names.size(); ++name_index) {
    const std::string &channel_name = channel_names[name_index];
    if (valid_channel_name_(channel_name)) {
      if (channels_.find(channel_name) != channels_.end()) {
        Channel &channel = channels_.find(channel_name)->second;
        check_priviliges(fd, client, channel, channel_key, &key_index);
      } else if (client.get_channels_list().size() >=
                 MAX_CHANNELS) //  user is in too many channels
        // Error 405 :You have joined too many channels
        queue_.push(std::make_pair(fd, numeric_reply_(405, fd, "")));
      else {
        // creating new channel and adding user
        channels_.insert(
            std::make_pair(channel_name, Channel(client_nick, channel_name)));
        client.add_channel(channel_name);
        Channel &channel = channels_.find(channel_name)->second;
        RPL_join(channel, client_nick);
        RPL_NAMREPLY(channel, client_nick, fd);
        RPL_ENDOFNAMES(client_nick, channel_name, fd);
      }
    } else
      // Error 403 :No such channel
      queue_.push(std::make_pair(fd, numeric_reply_(403, fd, channel_name)));
  }
}

bool Server::valid_channel_name_(const std::string &channel_name) const {
  size_t size = channel_name.size();
  if (size > 1 && size <= 200 &&
      (channel_name.at(0) == '#' || channel_name.at(0) == '&')) {
    size_t i = 1;
    while (i < size && channel_name.at(i) != ' ' && channel_name.at(i) != ',' &&
           channel_name.at(i) != (char)7 && channel_name.at(i) != '#' &&
           channel_name.at(i) != '&') {
      ++i;
    }
    if (i == size)
      return true;
  }
  return false;
}

/**
 * @brief fd quits the server and writes an appropriate PRIVMSG to all
 * channels that client has joined. The PRIVMSG is "Quit" if the message is
 * size 1. Otherwise, the last parameter is taken as the PRIVMSG.
 *
 * @param fd file descriptor of the client that is quitting
 * @param message message[0] = "QUIT", further arguments optional. Last argument
 * will be taken as quitting message to all channels
 */
void Server::quit_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  std::vector<std::string> channellist = client.get_channels_list();

  // Build quit message: "PRIVMSG" + "Channel[,Channel,Channel]" + "message"
  std::vector<std::string> quitmessage(1, "PRIVMSG");

  // check if exists ------------------------------------------------------
  if (channellist.size())
    quitmessage.push_back(channellist[0]);
  for (size_t i = 1; i < channellist.size(); ++i) {
    quitmessage[1] += ",";
    quitmessage[1] += channellist[i];
  }

  if (message.size() > 1) {
    quitmessage.push_back(message.at(message.size() - 1));
  } else {
    quitmessage.push_back("Quit");
  }

  privmsg_(fd, quitmessage);

  const std::string &clientname = client.get_nickname();
  for (size_t i = 0; i < channellist.size(); ++i) {
    channels_[channellist[i]].remove_user(clientname);
  }
  map_name_fd_.erase(client.get_nickname());
  disconnect_client_(fd);
}

/**
 * @brief Sends a private message to (list of) user(s) or channel(s)
 *
 * @param fd client who sends the message
 * @param message message[0] == "PRIVMSG", message[1] ==
 * "recipient[,recipient]", message[2] == "text to be sent"
 */
void Server::privmsg_(int fd, std::vector<std::string> &message) {
  if (message.size() == 1) {
    // Error 411: No recipient given
    queue_.push(std::make_pair(
        fd, numeric_reply_(411, fd, clients_[fd].get_nickname())));
    return;
  } else if (message.size() == 2) {
    // Error 412: No text to send
    queue_.push(std::make_pair(
        fd, numeric_reply_(412, fd, clients_[fd].get_nickname())));
    return;
  }

  std::vector<std::string> recipients = split_string(message[1], ',');

  for (size_t i = 0; i < recipients.size(); ++i) {
    // Recipient is channel (starts with '#' or '&')
    if (recipients[i].size() &&
        (recipients[i].at(0) == '#' || recipients[i].at(0) == '&')) {
      privmsg_to_channel_(fd, recipients[i], message[message.size() - 1]);
    }
    // Channel is a user
    else {
      privmsg_to_user_(fd, recipients[i], message[message.size() - 1]);
    }
  }
}

void Server::privmsg_to_channel_(int fd_sender, std::string channelname,
                                 std::string message) {
  // Channel not found
  if (channels_.find(channelname) == channels_.end()) {
    // Error 403: No such channel
    queue_.push(
        std::make_pair(fd_sender, numeric_reply_(403, fd_sender, channelname)));
    return;
  }
  Channel &channel = channels_[channelname];
  const Client &client = clients_[fd_sender];
  const std::string &clientname = client.get_nickname();

  // ADD check for -n flag

  if (channel.checkflag(C_MODERATED) && !channel.is_operator(clientname) &&
      !channel.is_speaker(clientname)) {
    // Error 404: Cannot send to channel
    queue_.push(
        std::make_pair(fd_sender, numeric_reply_(404, fd_sender, channelname)));
    return;
  }

  const std::vector<std::string> &userlist = channel.get_users();

  for (size_t i = 0; i < userlist.size(); ++i) {
    std::stringstream servermessage;
    std::string username = userlist[i];
    servermessage << username << " PRIVMSG " << channelname << " :" << message;
    queue_.push(std::make_pair(map_name_fd_[username], servermessage.str()));
  }
}

void Server::privmsg_to_user_(int fd_sender, std::string nickname,
                              std::string message) {
  if (map_name_fd_.find(nickname) == map_name_fd_.end()) {
    // Error 401: No such nick
    queue_.push(
        std::make_pair(fd_sender, numeric_reply_(401, fd_sender, nickname)));
    return;
  }

  std::stringstream servermessage;
  servermessage << ":" << clients_[fd_sender].get_nickname() << " PRIVMSG "
                << nickname << " " << message;
  queue_.push(std::make_pair(map_name_fd_[nickname], servermessage.str()));
}

/**
 * @brief Sends a NOTICE message to (list of) user(s) or channel(s)
 *
 * @param fd client who sends the message
 * @param message message[0] == "NOTICE", message[1] ==
 * "recipient[,recipient]", message[2] == "text to be sent"
 */
void Server::notice_(int fd, std::vector<std::string> &message) {
  if (message.size() < 3)
    return;

  std::vector<std::string> recipients = split_string(message[1], ',');

  for (size_t i = 0; i < recipients.size(); ++i) {
    // Recipient is channel (starts with '#' or '&')
    if (recipients[i].size() &&
        (recipients[i].at(0) == '#' || recipients[i].at(0) == '&')) {
      notice_to_channel_(fd, recipients[i], message[message.size() - 1]);
    }
    // Channel is a user
    else {
      notice_to_user_(fd, recipients[i], message[message.size() - 1]);
    }
  }
}

void Server::notice_to_channel_(int fd_sender, std::string channelname,
                                std::string message) {
  // Channel not found
  if (channels_.find(channelname) == channels_.end()) {
    return;
  }

  Channel &channel = channels_[channelname];
  const Client &client = clients_[fd_sender];
  const std::string &clientname = client.get_nickname();

  if (!channel.is_operator(clientname) && !channel.is_speaker(clientname))
    return;

  const std::vector<std::string> &userlist = channel.get_users();

  for (size_t i = 0; i < userlist.size(); ++i) {
    std::stringstream servermessage;
    std::string username = userlist[i];
    servermessage << username << " NOTICE " << channelname << " :" << message;
    queue_.push(std::make_pair(map_name_fd_[username], servermessage.str()));
  }
}

void Server::notice_to_user_(int fd_sender, std::string nickname,
                             std::string message) {
  if (map_name_fd_.find(nickname) == map_name_fd_.end())
    return;

  std::stringstream servermessage;
  servermessage << ":" << clients_[fd_sender].get_nickname() << " NOTICE "
                << nickname << " " << message;
  queue_.push(std::make_pair(map_name_fd_[nickname], servermessage.str()));
}

/**
 * @brief he KICK command can be  used  to  forcibly  remove  a  user  from  a
   channel. It 'kicks them out' of the channel (forced PART). Only a channel
 operator may kick another user out of a  channel.
 *
 * @param fd the client's file descriptor
 * @param message message[0] = "KICK", message[1] = <channel>, message[2] =
 <nickname> [, message[3] = <reason>]
 */
void Server::kick_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  const std::string &clientname = client.get_nickname();

  if (message.size() < 3) {
    // Error 461: Not enough parameters
    queue_.push(std::make_pair(fd, numeric_reply_(461, fd, clientname)));
    return;
  }

  const std::string &channelname = message[1];
  const std::string &victimname = message[2];

  // Is the channelname valid?
  if (!valid_channel_name_(channelname)) {
    // Error 476: Bad Channel Mask
    queue_.push(std::make_pair(fd, numeric_reply_(476, fd, channelname)));
    return;
  }

  std::map<std::string, Channel, irc_stringmapcomparator<std::string> >::iterator
      it = channels_.find(channelname);

  // Does the channel exist?
  if (it == channels_.end()) {
    // Error 403: No such channel
    queue_.push(std::make_pair(fd, numeric_reply_(403, fd, clientname)));
    return;
  }

  Channel &channel = (*it).second;

  if (!channel.is_user(clientname)) {
    // Error 442: You're not on that channel
    queue_.push(std::make_pair(fd, numeric_reply_(442, fd, channelname)));
    return;
  }
  if (!channel.is_operator(clientname)) {
    // Error 482: You're not channel operator
    queue_.push(std::make_pair(fd, numeric_reply_(482, fd, channelname)));
    return;
  }
  if (!channel.is_user(victimname)) {
    // Error 441: They aren't on that channel
    queue_.push(std::make_pair(
        fd, numeric_reply_(441, fd, victimname + " " + channelname)));
    return;
  }

  // Send kick message to channel
  std::stringstream servermessage;
  servermessage << ":" << clientname << " KICK " << channelname << " "
                << victimname << " :";
  if (message.size() == 3)
    servermessage << victimname;
  else {
    servermessage << message[3];
    for (size_t i = 4; i < message.size(); ++i) {
      servermessage << " " << message[i];
    }
  }
  send_message_to_channel_(channel, servermessage.str());

  // Finally kick them!
  channel.remove_user(victimname);
}

std::string Server::numeric_reply_(int error_number, int fd_client,
                                   std::string argument) {
  std::ostringstream ss;
  ss << ":" << server_name_ << " " << error_number << " "
     << clients_[fd_client].get_nickname();

  if (argument.size())
    ss << " ";

  ss << argument << " :" << error_codes_[error_number];
  return ss.str();
}

void Server::send_RPL_message_(int fd, int RPL_number,
                               const std::string &argument) {
  std::stringstream servermessage;
  servermessage << ":" << server_name_ << " " << RPL_number << " "
                << clients_[fd].get_nickname() << " " << argument;
  queue_.push(std::make_pair(fd, servermessage.str()));
}

/**
 * @brief TOPIC command: The TOPIC message is used to change or view the topic
 of a channel. The topic for channel <channel> is returned if there is no
 <topic> given.  If the <topic> parameter is present, the topic for that channel
 will be changed, if the channel modes permit this action.
 *
 * @param fd client's file descriptor
 * @param message message[0] = "TOPIC", message[1] = <channel> [, message[2] =
 <topic>]
 */
void Server::topic_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  const std::string &clientname = client.get_nickname();

  if (message.size() == 1) {
    // Error 461: Not enough parameters
    queue_.push(
        std::make_pair(fd, numeric_reply_(461, fd, client.get_nickname())));
    return;
  }

  std::string &channelname = message[1];
  std::map<std::string, Channel, irc_stringmapcomparator<std::string> >::iterator
      it = channels_.find(channelname);

  // Does the channel exist?
  if (it == channels_.end()) {
    // Error 403: No such channel
    queue_.push(std::make_pair(fd, numeric_reply_(403, fd, clientname)));
    return;
  }

  Channel &channel = (*it).second;
  if (message.size() == 2) {
    topic_send_info_(fd, channelname, channel);
  } else {
    topic_set_topic_(fd, channelname, channel, message[2]);
  }
}

/**
 * @brief Helper for topic_. If no topic is provided in the topic_ function
 * call, only information about the topic and the topicsetter is provided
 *
 * @param fd client's file descriptor
 * @param channelname channelname
 * @param channel channel object
 */
void Server::topic_send_info_(int fd, const std::string &channelname,
                              const Channel &channel) {
  const std::string &clientname = clients_[fd].get_nickname();
  std::stringstream prefix;
  prefix << ":" << server_name_ << " ";

  if (channel.is_topic_set()) {
    // RPL_TOPIC
    std::stringstream topicmessage;
    topicmessage << prefix.str() << "332 " << clientname << " " << channelname
                 << " :" << channel.get_topic_name();
    queue_.push(std::make_pair(fd, topicmessage.str()));
    // RPL_TOPICWHOTIME
    std::stringstream whomessage;
    whomessage << prefix.str() << "333 " << clientname << " " << channelname
               << " " << channel.get_topic_setter_name() << " "
               << channel.get_topic_set_time();
    queue_.push(std::make_pair(fd, whomessage.str()));
  } else {
    // RPL_NOTOPIC
    std::stringstream notopicmessage;
    notopicmessage << prefix.str() << "331 " << clientname << " " << channelname
                   << " :No topic is set.";
    queue_.push(std::make_pair(fd, notopicmessage.str()));
  }
}

/**
 * @brief Helper for topic_. If a topic is provided in the topic_ function -
 * provided the client has the rights - it is set or ,in case of an empty string
 * as the topic, cleared
 *
 * @param fd client's file descriptor
 * @param channelname channelname
 * @param channel channel object
 * @param topicname topicname
 */
void Server::topic_set_topic_(int fd, const std::string &channelname,
                              Channel &channel, const std::string &topicname) {
  const Client &client = clients_[fd];
  const std::string &clientname = client.get_nickname();
  if (channel.checkflag(C_TOPIC) && channel.is_operator(clientname)) {
    // Error 482: You're not channel operator
    queue_.push(std::make_pair(fd, numeric_reply_(482, fd, channelname)));
    return;
  }

  if (topicname.empty())
    channel.clear_topic();
  else
    channel.set_topic(topicname, clientname);

  std::stringstream servermessage;
  servermessage << ":" << server_name_ << " 332 " << clientname << " "
                << channelname << " :" << topicname;
  send_message_to_channel_(channel, servermessage.str());
}

void Server::mode_channel_(int fd, std::vector<std::string> &message,
                           Channel &channel) {
  Client &client = clients_[fd];
  if (!channel.get_operators().count(client.get_nickname())) {
    // check only for +b without arg flag
    check_plus_b_no_arg_flag(fd, message);
    // 482 You're not channel operator
    queue_.push(
        std::make_pair(fd, numeric_reply_(482, fd, channel.get_channelname())));
    return;
  }
  // For giving info at the end
  std::vector<char> added_modes, removed_modes;
  std::vector<std::string> added_mode_arguments, removed_mode_arguments;

  // For parsing the modestring and matching it with the arguments
  bool sign = true;
  std::string &modestring = message[2];
  std::vector<std::string>::iterator argument_iterator(&(message[3]));
  std::vector<std::string>::iterator end_iterator = message.end();

  for (size_t i = 0; i < modestring.size(); ++i) {
    char current = modestring.at(i);

    if (current == '+') {
      sign = true;
    } else if (current == '-') {
      sign = false;
    } else if (mode_functions_.find(current) != mode_functions_.end()) {
      // Execute the abomination of a function pointer in the map
      std::pair<size_t, std::string> ret = (this->*mode_functions_[current])(
          fd, channel, sign, argument_iterator, end_iterator);
      // If successful, add it to the vector to give information at the end
      // For loop especially for MODE -b, when removing several bans
      if (ret.first) {
        for (size_t i = 0; i < ret.first; ++i) {
          sign ? added_modes.push_back(current)
               : removed_modes.push_back(current);
        }
      }
      // If it was a successfull command that required an argument, the argument
      // is then added to the vector to give information at the end
      if (!ret.second.empty()) {
        sign ? added_mode_arguments.push_back(ret.second)
             : removed_mode_arguments.push_back(ret.second);
      }
    } else {
      // Error 472: is unknown mode char to me
      queue_.push(
          std::make_pair(fd, numeric_reply_(472, fd, std::string(1, current))));
    }
  }
  // If one or more commands were successful, send an info message to the
  // whole channel
  if (!added_modes.empty() || !removed_modes.empty()) {
    mode_channel_successmessage_(fd, channel, added_modes, removed_modes,
                                 added_mode_arguments, removed_mode_arguments);
  }
}

void Server::mode_channel_successmessage_(
    int fd, Channel &channel, std::vector<char> &added_modes,
    std::vector<char> &removed_modes,
    std::vector<std::string> &added_mode_arguments, std::vector<std::string> &removed_mode_arguments) {
  std::stringstream servermessage;
  servermessage << ":" << clients_[fd].get_nickname() << " MODE "
                << channel.get_channelname() << " ";

  if (!removed_modes.empty()) {
    servermessage << "-";
    for (size_t i = 0; i < removed_modes.size(); ++i)
      servermessage << removed_modes[i];
  }

  if (!added_modes.empty()) {
    servermessage << "+";
    for (size_t i = 0; i < added_modes.size(); ++i)
      servermessage << added_modes[i];
  }

  if (!removed_mode_arguments.empty()) {
    for (size_t i = 0; i < removed_mode_arguments.size(); ++i)
      servermessage << " " << removed_mode_arguments[i];
  }

  if (!added_mode_arguments.empty()) {
    for (size_t i = 0; i < added_mode_arguments.size(); ++i)
      servermessage << " " << added_mode_arguments[i];
  }

  send_message_to_channel_(channel, servermessage.str());
}

std::pair<size_t, std::string> Server::mode_channel_o_(
    int fd, Channel &channel, bool plus,
    std::vector<std::string>::iterator &arg,
    std::vector<std::string>::iterator &end) {
  if (arg == end) {
    // if no argument is given, ignore silently
    return std::make_pair(0, "");
  }

  std::string &name = *(arg++);
  if (!channel.is_user(name)) {
    // Error 401: No such nick
    queue_.push(std::make_pair(fd, numeric_reply_(401, fd, name)));
    return std::make_pair(0, "");
  }

  // +o
  if (plus) {
    if (channel.is_operator(name)) {
      // ignore silently
      return std::make_pair(0, "");
    }
    channel.add_operator(name);
    return std::make_pair(1, name);
  }
  // -o
  else {
    if (channel.is_operator(name)) {
      channel.remove_operator(name);
      return std::make_pair(1, name);
    }
    // ignore silently
    return std::make_pair(0, "");
  }
}

std::pair<size_t, std::string>
Server::mode_channel_i_(int fd, Channel &channel, bool plus,
                        std::vector<std::string>::iterator &arg,
                        std::vector<std::string>::iterator &end) {

  // if the mode is already set to that version then return silently
  if ((plus && channel.checkflag(C_INVITE)) ||
      (!plus && !channel.checkflag(C_INVITE))) {
    return std::make_pair(0, std::string());
  } else if (plus) {
    channel.setflag(C_INVITE);
    return std::make_pair(1, std::string());
  } else {
    channel.clearflag(C_INVITE);
    return std::make_pair(1, std::string());
  }
  (void)fd;
  (void)end;
  (void)arg;
  return std::make_pair(0, std::string());
}

std::pair<size_t, std::string>
Server::mode_channel_t_(int fd, Channel &channel, bool plus,
                        std::vector<std::string>::iterator &arg,
                        std::vector<std::string>::iterator &end) {
  // if the mode is already set to that version then return silently
  if ((plus && channel.checkflag(C_TOPIC)) ||
      (!plus && !channel.checkflag(C_TOPIC))) {
    return std::make_pair(0, std::string());
  } else if (plus) {
    channel.setflag(C_TOPIC);
    return std::make_pair(1, std::string());
  } else {
    channel.clearflag(C_TOPIC);
    return std::make_pair(1, std::string());
  }
  (void)fd;
  (void)end;
  (void)arg;
  return std::make_pair(0, std::string());
}

std::pair<size_t, std::string>
Server::mode_channel_m_(int fd, Channel &channel, bool plus,
                        std::vector<std::string>::iterator &arg,
                        std::vector<std::string>::iterator &end) {
  if ((plus && channel.checkflag(C_MODERATED)) ||
      (!plus && !channel.checkflag(C_MODERATED))) {
    return std::make_pair(0, std::string());
  } else if (plus) {
    channel.setflag(C_MODERATED);
    return std::make_pair(1, std::string());
  } else {
    channel.clearflag(C_MODERATED);
    return std::make_pair(1, std::string());
  }
  (void)fd;
  (void)end;
  (void)arg;
  return std::make_pair(0, std::string());
}

std::pair<size_t, std::string>
Server::mode_channel_l_(int fd, Channel &channel, bool plus,
                        std::vector<std::string>::iterator &arg,
                        std::vector<std::string>::iterator &end) {

  // if "-l"
  if (plus == false) {
    if (channel.get_user_limit() == MAX_CLIENTS) {
      return std::make_pair(0, std::string());
    } else {
      channel.set_user_limit(MAX_CLIENTS);
      return std::make_pair(1, std::string());
    }
  }
  // if "+l"
  else {
    if (arg == end) {
      // 461 not enough parameters
      queue_.push(std::make_pair(fd, numeric_reply_(461, fd, "MODE +l")));
      return std::make_pair(0, std::string());
    }
    std::string tmp_arg = (*arg);
    int newlimit;
    if (!is_valid_userlimit(tmp_arg)) {
      newlimit = 0;
    } else {
      newlimit = atoi(tmp_arg.c_str());
    }
    arg++;
    if (newlimit <= 0) {
      return std::make_pair(0, std::string());
    }
    if (newlimit > MAX_CLIENTS) {
      return std::make_pair(0, std::string());
    }
    if ((size_t)newlimit == channel.get_user_limit()) {
      return std::make_pair(0, std::string());
    }
    channel.set_user_limit(newlimit);
    return std::make_pair(1, tmp_arg);
  }
  (void)fd;
}

std::pair<size_t, std::string> Server::mode_channel_b_(
    int fd, Channel &channel, bool plus,
    std::vector<std::string>::iterator &arg,
    std::vector<std::string>::iterator &end) {
  // No argument? Print a list of users
  if (arg == end) {
    mode_channel_b_list_(fd, channel);
    return std::make_pair(0, "");
  }
  // +b: ban a mask
  if (plus) {
    return mode_channel_b_add_banmask_(fd, channel, arg);
  }
  // -b: remove all banmasks that fit the argument
  else {
    return channel.remove_banmask(*arg);
  }
}

void Server::mode_channel_b_list_(int fd, const Channel &channel) {
  const std::vector<banmask> &list_banmasks = channel.get_banned_users();
  std::stringstream prefixstream;
  prefixstream << ":" << server_name_ << " 367" << clients_[fd].get_nickname()
               << " " << channel.get_channelname() << " ";
  std::string prefix(prefixstream.str());

  // List all banmasks with RPL_BANLIST (367)
  for (size_t i = 0; i < list_banmasks.size(); ++i) {
    std::stringstream servermessage;
    const banmask &current = list_banmasks[i];
    servermessage << prefix << current.banned_nickname << "!"
                  << current.banned_username << "@" << current.banned_hostname
                  << " " << current.banned_by << " " << current.time_of_ban;
    queue_.push(std::make_pair(fd, servermessage.str()));
  }

  // End with RPL_ENDBANLIST (368)
  std::stringstream servermessage;
  servermessage << ":" << server_name_ << " 368 " << clients_[fd].get_nickname()
                << " " << channel.get_channelname()
                << " :End of Channel Ban List";
  queue_.push(std::make_pair(fd, servermessage.str()));
}

std::pair<size_t, std::string> Server::mode_channel_b_add_banmask_(
    int fd, const Channel &channel, std::vector<std::string>::iterator &arg) {
  std::string banmask_nickname;
  std::string banmask_username;
  std::string banmask_hostname;
  parse_banmask(*arg, banmask_nickname, banmask_username, banmask_hostname);

  // Is the banmask already covered by the existing masks?
  const std::vector<banmask> &list_banmasks = channel.get_banned_users();
  for (size_t i = 0; i < list_banmasks.size(); ++i) {
    const banmask &current = list_banmasks[i];
    if (irc_wildcard_cmp(current.banned_nickname.c_str(),
                         banmask_nickname.c_str()) &&
        irc_wildcard_cmp(current.banned_username.c_str(),
                         banmask_username.c_str()) &&
        irc_wildcard_cmp(current.banned_hostname.c_str(),
                         banmask_hostname.c_str()))
      return std::make_pair(0, "");
  }

  banmask tmp;
  tmp.banned_nickname = banmask_nickname;
  tmp.banned_username = banmask_username;
  tmp.banned_hostname = banmask_hostname;
  tmp.banned_by = clients_[fd].get_nickname();
  tmp.time_of_ban = time(NULL);

  std::stringstream new_mask;
  new_mask << banmask_nickname << "!" << banmask_username << "@"
           << banmask_hostname;
  return std::make_pair(1, new_mask.str());
}

std::pair<size_t, std::string>
Server::mode_channel_v_(int fd, Channel &channel, bool plus,
                        std::vector<std::string>::iterator &arg,
                        std::vector<std::string>::iterator &end) {
  if (arg == end) {
    // Error 461: Not enough parameters
    queue_.push(std::make_pair(
        fd, numeric_reply_(461, fd, clients_[fd].get_nickname())));
    return std::make_pair(0, "");
  }
  std::string nickname = (*arg);
  arg++;
  // if the nickname is not valid
  if (!channel.is_user(nickname)) {
    queue_.push(std::make_pair(fd, numeric_reply_(401, fd, nickname)));
    return std::make_pair(0, std::string());
  }
  // if the user is not on the speaker list
  if (!channel.get_speakers().count(nickname)) {
    if (plus) {
      channel.add_speaker(nickname);
      return std::make_pair(1, nickname);
    } else {
      return std::make_pair(0, std::string());
    }
  }
  // if the user is on the speaker list
  else {
    if (!plus) {
      channel.remove_speaker(nickname);
      return std::make_pair(1, nickname);
    } else {
      return std::make_pair(0, std::string());
    }
  }
  return std::make_pair(0, std::string());
}

std::pair<size_t, std::string>
Server::mode_channel_k_(int fd, Channel &channel, bool plus,
                        std::vector<std::string>::iterator &arg,
                        std::vector<std::string>::iterator &end) {
  if (arg == end) {
    // Error 461: Not enough parameters
    queue_.push(std::make_pair(
        fd, numeric_reply_(461, fd, clients_[fd].get_nickname())));
    return std::make_pair(0, "");
  }
  std::string &key = *(arg++);
  // if +k
  if (plus) {
    // If password is already set, give an error
    if (!channel.get_channel_password().empty()) {
      // Error 467: Channel key already set
      queue_.push(std::make_pair(
          fd, numeric_reply_(467, fd, channel.get_channelname())));
      return std::make_pair(0, "");
    }
    if (!channel_key_is_valid(key)) {
      // Error 525: Key is not well-formed
      queue_.push(std::make_pair(
          fd, numeric_reply_(525, fd, channel.get_channelname())));
      return std::make_pair(0, "");
    }
    channel.set_channel_password(key);
    return std::make_pair(1, key);
  }
  // else -k
  else {
    if (key == channel.get_channel_password()) {
      // Error 467: Channel key already set <- strange error, but quakenet sends
      // this
      queue_.push(std::make_pair(
          fd, numeric_reply_(467, fd, channel.get_channelname())));
      return std::make_pair(0, "");
    }
    std::string emptypw;
    channel.set_channel_password(emptypw);
    return std::make_pair(1, "");
  }
}

void Server::check_plus_b_no_arg_flag(int fd,
                                      std::vector<std::string> &message) {
  bool sign = true;
  std::string &modestring = message[2];
  std::vector<std::string>::iterator arg(&(message[3]));
  std::vector<std::string>::iterator end = message.end();

  for (size_t i = 0; i < modestring.size(); ++i) {
    char current = modestring.at(i);

    if (current == '+') {
      sign = true;
    } else if (current == '-') {
      sign = false;
    }
      else if (current == 'o' || current == 'b' || current == 'v' ||
               current == 'k' || (sign && current == 'l')) {
        if (arg != end) {
          arg++;
        } else if (sign && current == 'b') {
          queue_.push(std::make_pair(fd, "DEBUG: printing ban list"));
          // mode_channel_b_list_(fd, channel);
        }
      }
    }
  }

} // namespace irc