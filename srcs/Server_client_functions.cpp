#include "Server.hpp"

namespace irc {

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
    if (client.is_authorized()) welcome_(fd);
  } else {
    // Error 464: password incorrect
    queue_.push(std::make_pair(fd, numeric_reply_(464, fd, message[1])));
  }
}

bool Server::search_nick_list(std::string nick) {
  std::map<int, Client>::iterator it;
  for (it = clients_.begin(); it != clients_.end(); ++it) {
    if (irc_stringissame(nick, (*it).second.get_nickname())) {
      return 1;
    }
  }
  return 0;
}

int Server::search_user_list(std::string user) {
  std::map<int, Client>::iterator it;
  for (it = clients_.begin(); it != clients_.end(); ++it) {
    if (user == (*it).second.get_username()) {
      return (*it).first;
    }
  }
  return -1;
}

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
  if (client.is_authorized()) welcome_(fd);
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

void Server::nick_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  #if DEBUG
    std::cout <<"inside nick funcion\n";
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
    //432 erroneous nickname
    queue_.push(std::make_pair(fd, numeric_reply_(432, fd, message[1])));
    return ;
  }
  if (search_nick_list(message[1])) {
    // Error 433: Nickname is already in use
    queue_.push(
        std::make_pair(fd, numeric_reply_(433, fd, client.get_nickname())));
    return;
  }
  client.set_nickname(message[1]);
  map_name_fd_.insert(std::make_pair(message[1], fd));
  if (!client.get_status(NICK_AUTH)) {
    client.set_status(NICK_AUTH);
    if (client.is_authorized()) welcome_(fd);
  }
}

void Server::pong_(int fd, std::vector<std::string> &message) {
  if (message.size() != 2) return;

  Client &client = clients_[fd];
  if (client.get_expected_ping_response() == message[1]) {
    if (!client.get_status(PONG_AUTH)) {
      client.set_status(PONG_AUTH);
      if (client.is_authorized()) welcome_(fd);
    }
    client.set_pingstatus(true);
  }
}

/**
 * @brief the user leaves all channels specified in the parameter
 *
 * @param fd the client's file descriptor
 * @param message message[0] == "PART", message[1] ==
 * "channelname[,channelname]"
 */
void Server::part_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  if (message.size() < 2) {
    // Error 461: Not enough parameters
    queue_.push(
        std::make_pair(fd, numeric_reply_(461, fd, client.get_nickname())));
    return;
  }

  std::vector<std::string> channellist = split_string(message[1], ',');

  for (size_t i = 0; i < channellist.size(); ++i) {
    std::map<std::string, Channel,
             irc_stringmapcomparator<std::string> >::iterator it =
        channels_.find(channellist[i]);

    if (it == channels_.end()) {
      // Error 403: No such channel
      queue_.push(
          std::make_pair(fd, numeric_reply_(403, fd, client.get_nickname())));
      continue;
    }
    // Channel &channel = (*it).second;
    // channel.remove_user(client.get_nickname());
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
                  << " ircserv 1.0 iswo opsitnmlbvk olbvk";
    queue_.push(std::make_pair(fd, servermessage.str()));
  }

  // 005 RPL_ISUPPORT
  {
    std::stringstream servermessage;
    servermessage << ":" << server_name_ << " 002 " << clientname
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

    if (!(*it).second.is_authorized()) ++n_unauthorized;

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
    queue_.push(std::make_pair(fd, numeric_reply_(402, fd, " " + message[1])));
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
    if (infile.fail()) throw std::exception();
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

void Server::join_(int fd, std::vector<std::string> &message) {
  #if DEBUG
    std::cout << "Join function called" << std::endl;
    std::cout << "FD: " << fd << std::endl;
    for (size_t i = 0; i < message.size(); ++i)
      std::cout << "Message " << i << ": " << message[i] << std::endl;
  #endif

  if (!message[1])
    #if DEBUG
        std::cout << "Error 461 :Not enough parameters" << std::endl;
    #endif
    // Error 461 :Not enough parameters

  std::vector<std::string>  channel_name_ = split_string(message[1], ';');
  std::vector<std::string>  channel_key_  = split_string(message[2], ';');
  if (channels_.find(channel_name_[0]) != channels_.end()) {
    Channel temp = (channels_.find(channel_name_[0]))->second;                             // check if user is already in channel?
    if (temp.checkflag(C_INVITE) && 1 /* !(clients_[fd].is_invited(channel_name_[0])) */)
      #if DEBUG
        std::cout << "Error 473 :Cannot join channel (+i)" << std::endl;
      #endif
      // Error 473 :Cannot join channel (+i)
    else if (temp.is_banned(clients_[fd].get_nickname()))
      #if DEBUG
        std::cout << "Error 473 :Cannot join channel (+b)" << std::endl;
      #endif
      // Error 474 :Cannot join channel (+b)
    else if (temp.get_channel_password() != "" && !irc_stringissame(temp.get_channel_password(), channel_key_[0]))
      #if DEBUG
        std::cout << "Error 473 :Cannot join channel (+k)" << std::endl;
      #endif
      // Error 475 :Cannot join channel (+k)
    else if (temp.get_users().size() >= temp.get_user_limit())
      #if DEBUG
        std::cout << "Error 473 :Cannot join channel (+l)" << std::endl;
      #endif
      // Error 471 :Cannot join channel (+l)

    // else if (clients_[fd].get_channels_list().size() >= max_channel_size)
      // Error 405 :You have joined too many channels

    //  adding clients by user or nick name?
  }
  else
    #if DEBUG
        std::cout << "Error 403 :No such channel" << std::endl;
    #endif
    // Error 403 :No such channel
}

void Server::remove_channel_(int fd, std::vector<std::string> &message) {
  // Check validity of message (size, parameters, etc...)
  clients_[fd].remove_channel(message[1]);
}

/**
 * @brief fd quits the server and writes an appropriate PRIVMSG to all
 * channels that client has joined. The PRIVMSG is "Quit" if the message is
 * size 1. Otherwise, the last parameter is taken as the PRIVMSG.
 *
 * @param fd file descriptor of the client that is quitting
 * @param message message[0] = "QUIT", further arguments optional. Last argument
 * qill be taken as quitting message to all channels
 */
void Server::quit_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  std::vector<std::string> channellist = client.get_channels_list();

  // Build quit message: "PRIVMSG" + "Channel[,Channel,Channel]" + "message"
  std::vector<std::string> quitmessage(1, "PRIVMSG");

  if (channellist.size()) quitmessage.push_back(channellist[0]);
  for (size_t i = 1; i < channellist.size(); ++i) {
    quitmessage[1] += ",";
    quitmessage[1] += channellist[i];
  }

  if (message.size() > 1) {
    quitmessage.push_back(message.at(message.size() - 1));
  } else {
    quitmessage.push_back("Quit");
  }

  // Send quitting message and disconnect client from server
  // privmsg_(fd, quitmessage);
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
  if (0 /* !channel.is_operator(fd_sender) && channel.is_on_mutedlist(fd_sender)*/) {
    // Error 404: Cannot send to channel
    queue_.push(
        std::make_pair(fd_sender, numeric_reply_(404, fd_sender, channelname)));
  }

  // Channel &channel = channels_[channelname];
  std::vector<std::string> userlist(1, "TESTUSER");  // = channel.get_users_();

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
  servermessage << ":" << clients_[fd_sender].get_nickname() << " PRIVMSG"
                << nickname << " " << message;
  queue_.push(std::make_pair(map_name_fd_[nickname], servermessage.str()));
}

std::string Server::numeric_reply_(int error_number, int fd_client,
                                   std::string argument) {
  std::ostringstream ss;
  ss << ":" << server_name_ << " " << error_number << " "
     <<clients_[fd_client].get_nickname() <<  argument << " :"
     << error_codes_[error_number];
  return ss.str();
}

}  // namespace irc