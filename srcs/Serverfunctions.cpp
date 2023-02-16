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
  int user_fd = search_user_list(message[1]);
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

int readsign(std::string argument) {

  if (argument.size() < 2) {
    std::cout << "bad argument\n";
    return -1;
  }
  if (argument.at(0) == '-')
    return 0;
  else if (argument.at(0) == '+')
    return 1;
  else {
    std::cout << "no +/- before the flag(s)\n";
    return -1;
  }
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
  } else {
    if (client.get_nickname() != message[1]) {
      if (search_nick_list(message[1]) == 0) {
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
    if (message[2].size() < 2) {
      // 501 unknown MODE flag
      queue_.push(std::make_pair(fd, numeric_reply_(501, fd, message[2])));
      return;
    }
    // maybe it would be good to add other flags at least to ignore them
    if (message[2] == "+o")
      return;
    else if (message[2] == "-o") {
      client.set_server_operator_status(0);
      return;
    } else {
      // 501 err_ unknownmode
      queue_.push(std::make_pair(fd, numeric_reply_(501, fd, message[2])));
      return;
    }
  }
}

} // namespace irc