#include "Client.hpp"
#include "Server.hpp"

namespace irc {

void Server::oper_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  if (message.size() < 3) {
    // Error 461: Not enough parameters
    queue_.push(
        std::make_pair(fd, numeric_reply_(461, fd, client.get_nickname())));
    return;
  }
  int user_fd = search_user_list_(message[1]);
  if (user_fd < 0) {
    queue_.push(
        // 444 User not logged in (cant find username)
        std::make_pair(fd, numeric_reply_(444, fd, message[1])));
    return;
  } else if (clients_[user_fd].get_server_operator_status() == 1) {
    // that user is already an operator
    return;
  }
  if (message[2] == operator_password_) {
    // 381 You are now an IRC operator
    clients_[user_fd].set_server_operator_status(1);
    queue_.push(std::make_pair(
        fd, numeric_reply_(381, fd, clients_[user_fd].get_username())));
  } else {
    // 464 password incorrect
    queue_.push(std::make_pair(fd, numeric_reply_(464, fd, message[2])));
  }
}

bool Server::validflags_(int fd, std::string flags) {
  Client &client = clients_[fd];
  if (flags.size() < 2)
    return false;
  if (flags == "+o")
    return true;
  else if (flags == "-o") {
    client.set_server_operator_status(0);
    return true;
  } else if (flags == "+s" || flags == "+os" || flags == "+so") {
    client.set_server_notices_status(1);
    return true;
  } else if (flags == "-s") {
    client.set_server_notices_status(0);
    return true;
  } else if (flags == "-os" || flags == "-so") {
    client.set_server_notices_status(0);
    client.set_server_operator_status(0);
    return true;
  } else
    return false;
}

void Server::mode_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  if (message.size() < 3) {
    // Error 461: Not enough parameters
    queue_.push(
        std::make_pair(fd, numeric_reply_(461, fd, client.get_nickname())));
    return;
  }

  if (message[1].at(0) == '#') {
    std::cout << "channel command, in progress\n";
    // mode + user
  } else {
    // server command
    // check for correct syntax
    if (!irc_stringissame(client.get_nickname(), message[1])) {
      if (search_nick_list_(message[1]) == 0) {
        // 401 no such nickname
        queue_.push(std::make_pair(fd, numeric_reply_(401, fd, message[2])));
        return;
      } else {
        // 502 can't change mode for other users
        queue_.push(
            std::make_pair(fd, numeric_reply_(502, fd, client.get_nickname())));
        return;
      }
    }
    // check for valid flags
    if (validflags_(fd, message[2]) == false) {
      // 501 err_ unknownmode
      queue_.push(std::make_pair(fd, numeric_reply_(501, fd, message[2])));
    }
  }
}

void Server::kill_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  if (message.size() < 3) {
    // Error 461: Not enough parameters
    queue_.push(
        std::make_pair(fd, numeric_reply_(461, fd, client.get_nickname())));
    return;
  }
  if (!client.get_server_operator_status()) {
    //481,"Permission Denied- You're not an IRC operator" 
    queue_.push(
            std::make_pair(fd, numeric_reply_(481, fd, client.get_nickname())));
    return;
  }
  if (!search_nick_list_(message.at(1))) {
    // 401 no such nickname
    queue_.push(std::make_pair(fd, numeric_reply_(401, fd, message[1])));
    return;
  }
  int victimfd = map_name_fd_[message[1]];
  //queue_.push(std::make_pair(victimfd, numeric_reply_(361, "")));
  std::vector<std::string> quitmessage(1, "QUIT");
  quitmessage.push_back("Killed(" + client.get_nickname() + "(" + message[2] + "))");
  quit_(victimfd, quitmessage);
}

} // namespace irc