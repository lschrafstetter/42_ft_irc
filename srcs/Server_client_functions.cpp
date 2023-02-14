#include "Client.hpp"
#include "Server.hpp"

namespace irc {

// called when the user enters PASS
void Server::authenticate_password_(int fd, std::vector<std::string> &message) {
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

void Server::set_username_(int fd, std::vector<std::string> &message) {
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
  //: server 468 nick :Your username is invalid.
  //: server 468 nick :Connect with your real username, in lowercase.
  //: server 468 nick :If your mail address were foo@bar.com, your username
  //: would be foo.
  // return ;
  clients_[fd].set_username(message[1]);
  clients_[fd].set_auth_status(USER_AUTH);
}

void Server::set_nickname_(int fd, std::vector<std::string> &message) {
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