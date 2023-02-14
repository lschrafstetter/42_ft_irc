#include "Client.hpp"
#include "Server.hpp"

namespace irc {

// called when the user enters PASS
void Server::pass_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  if (client.is_fully_authorized() == 1) {
    std::cout << "Password already authenticated\n";
    return;
  }
  if (message.size() == 2 && message[1] == password_) {
    std::cout << "Password authenticated; access permitted\n";
    clients_[fd].set_auth_status(PASS_AUTH);
  } else {
    std::cout << "Password incorrect; access denied\n";
  }
}

bool Server::search_nick_list(std::string nick) {
  std::string error_str;
  std::map<int, Client>::iterator it;
  for (it = clients_.begin(); it != clients_.end(); ++it) {
    if (nick == (*it).second.get_nickname()) {
      return 1;
    }
  }
  return 0;
}

void Server::user_(int fd, std::vector<std::string> &message) {
  // Work in progress
  // std::string error_msg;
  if (clients_[fd].get_auth_status(USER_AUTH)) {
    // error_msg = ":irc 462" + clients_[fd].get_nickname() + " :You may not
    // reregister";
    queue_.push(std::make_pair(fd, numeric_reply_(462, fd)));
    return;
  }
  if (message.size() < 4) {
    // error_msg = ":irc 461 " + clients_[fd].get_username() + " :Not enough
    // parameters";
    queue_.push(std::make_pair(fd, numeric_reply_(461, fd)));
    return;
  }
  // check for a valid username
  //:server 468 nick :Your username is invalid.
  //:server 468 nick :Connect with your real username, in lowercase.
  //:server 468 nick :If your mail address were foo@bar.com, your username would
  // be foo. return ;
  clients_[fd].set_username(message[1]);
  clients_[fd].set_auth_status(USER_AUTH);
}

void Server::nick_(int fd, std::vector<std::string> &message) {
  std::string error_str;
  if (search_nick_list(message[1])) {
    error_str =
        ":irc " + clients_[fd].get_nickname() + " :Nickname is already in use.";
    queue_.push(std::make_pair(fd, error_str));
    return;
  }
  /* if (nickname_exists) {
    queue_.push(std::make_pair(fd, "ERR_NICKCOLLISION"));
    return;
  } */
  // Check list of nicknames!
  // Work in progress
  clients_[fd].set_nickname(message[1]);
  clients_[fd].set_auth_status(NICK_AUTH);
}

void Server::pong_(int fd, std::vector<std::string> &message) {
  if (message.size() != 2) return;

  Client &client = clients_[fd];
  if (client.get_expected_ping_response() == message[1]) {
    client.set_pingstatus(true);
    // client.set_auth_status(PONGFLAG)
  }

  // additionally check for authentication status??
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
 * @param message message[0] = "PRIVMSG", message[1] = recipient[,recipient],
 * message[2] = "text to be sent"
 */
void Server::privmsg_(int fd, std::vector<std::string> &message) {
  if (message.size() == 1) {
    // Send error code 411: ":ircserv 411 [NICKNAME] :No recipient given
    // (PRIVMSG)"
    return;
  } else if (message.size() == 2) {
    // Send error code 412: ":ircserv 412 [NICKNAME] :No text to send"
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
    // Send error code 403: ":ircserv 403 [NICKNAME] [channelname] :No such
    // channel"
    return;
  }

  std::vector<std::string> userlist(1, "TESTUSER"); // = channel.get_users_();
  std::stringstream servermessage;

  for (size_t i = 0; i < userlist.size(); ++i) {
    servermessage.clear();
  }
}

void Server::privmsg_to_user_(int fd_sender, std::string username,
                              std::string message) {
  if (map_name_fd_.find(username) == map_name_fd_.end()) {
    // Send error code 403: ":ircserv 401 [nickname] [recipient_name] :No such
    // nick
    return;
  }

  std::stringstream servermessage;
  servermessage << ":" << clients_[fd_sender].get_nickname() << " PRIVMSG"
                << username << " " << message;
  queue_.push(std::make_pair(map_name_fd_[username], servermessage.str()));
}

void Server::init_error_codes_() {
  error_codes_.insert(
      std::make_pair<int, std::string>(462, "You may not reregister"));
  error_codes_.insert(
      std::make_pair<int, std::string>(461, "Not enough parameters"));
}

std::string Server::numeric_reply_(int numeric, int fd) {
  std::ostringstream ss;
  ss << numeric;
  return (":" + server_name_ + " " + ss.str() + " " +
          clients_[fd].get_nickname() + " :" + error_codes_[numeric]);
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