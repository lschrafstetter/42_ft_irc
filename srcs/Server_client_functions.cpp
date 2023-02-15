#include "Client.hpp"
#include "Server.hpp"
bool irc_stringissame(const std::string & str1, const std::string & str2);

namespace irc {

void Server::pass_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  if (client.get_status(PASS_AUTH) == 1) {
    queue_.push(std::make_pair(fd, numeric_reply_(462, fd, client.get_nickname())));
    return;
  }
  if (message.size() == 1) {
      queue_.push(std::make_pair(fd, numeric_reply_(461, fd, client.get_nickname())));
      return;
  }
  if (message.size() == 2 && message[1] == password_) {
    #if DEBUG
      std::cout << "Password accepted; access permitted\n";
    #endif
    client.set_status(PASS_AUTH);
  } else {
    //error 464 password incorrect
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

void Server::user_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  if (!client.get_status(PASS_AUTH)) {
    queue_.push(std::make_pair(fd, numeric_reply_(464, fd, "Enter password")));
    return ;
  }
  if (client.get_status(USER_AUTH)) {
    queue_.push(std::make_pair(fd, numeric_reply_(462, fd, "Enter password")));
    return;
  }
  if (message.size() < 4) {
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
}

void Server::nick_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  if (!client.get_status(PASS_AUTH)) {
    queue_.push(std::make_pair(fd, numeric_reply_(464, fd, client.get_nickname())));
    return;
  }
  if (message.size() == 1) {
    queue_.push(std::make_pair(fd, numeric_reply_(461, fd,client.get_nickname() )));
    return ;
  }
  if (search_nick_list(message[1]))  {
      queue_.push(std::make_pair(fd, numeric_reply_(433, fd, client.get_nickname())));
      return;
  }
  client.set_nickname(message[1]);
  map_name_fd_.insert(std::make_pair(message[1], fd));
  client.set_status(NICK_AUTH);
}

void Server::pong_(int fd, std::vector<std::string> &message) {
  if (message.size() != 2) return;

  Client &client = clients_[fd];
  if (client.get_expected_ping_response() == message[1]) {
    client.set_pingstatus(true);
    client.set_status(PONG_AUTH);
  }
  // additionally check for authentication status??
}

// void Server::join_channel_(int fd, std::vector<std::string> &message) {
//   std::vector<std::string>  channel_name_ = split_std_strings(message[1], ';');
//   std::vector<std::string>  channel_key_  = split_std_strings(message[2], ';');
//   // std::string               buf;
//   // while (std::getline(message[1], buf, ','))
//   //   channel_name_.push_back(buf);
//   // while (std::getline(message[2], buf, ','))
//   //   channel_key_.push_back(buf);
//   std::map<std::string, Channel>::iterator	it = channels_.find(channel_name_[0]);
//   if (it != channels_.end()) {
//     it->second.get_users_().size() < 
//     /*  compare channel flags with user/client?!
//     **  add user/client to channel and channel to user/client
//     */
//   }
//     /*  what does the message look like? any parsing needed?
//     **  create new variables for storing information?
//     **  if (message == channel_name_) {
//     **  checkflag(C_PRIVATE) && user_authentification
//     **  }
//     */
// }

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
 * @param message message[0] = "PRIVMSG", message[1] = recipient[,recipient],
 * message[2] = "text to be sent"
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

  //Channel &channel = channels_[channelname];
  std::vector<std::string> userlist(1, "TESTUSER");  // = channel.get_users_();
  std::stringstream servermessage;

  for (size_t i = 0; i < userlist.size(); ++i) {
    servermessage.clear();
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
     << clients_[fd_client].get_nickname() << argument << " :"
     << error_codes_[error_number];
  return ss.str();
}

/*
void Server::try_create_operator_(int fd, std::vector<std::string> &message) {
        if (client.get_server_operator_status() == 1) {
                return ;
        }
        if (password == operator_password_) {
                client.set_server_operator_status(1);
                std::cout <<"Password correct, " <<client.get_nickname() <<" now
has operator status\n";
        }
        else {
                std::cout <<"Password incorrect, operator status cannot be
given\n";
        }
}

void Server::remove_operator_(int fd, std::vector<std::string> &message) {
        if (client.get_server_operator_status() == 0) {
                return ;
        }
        if (password == operator_password_) {
                client.set_server_operator_status(0);
                std::cout <<"Operator status removed\n";
        }
        else {
                std::cout <<"Password incorrect, " <<client.get_nickname() <<"
still is an operator\n";
        }
} */

}  // namespace irc