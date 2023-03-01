#include "Server.hpp"

namespace irc {

void Server::oper_(int fd, std::vector<std::string> &message) {
  if (message.size() < 3) {
    // Error 461: Not enough parameters
    queue_.push(std::make_pair(fd, numeric_reply_(461, fd, "OPER")));
    return;
  }
  std::string username = message[1];
  if (!map_name_fd_.count(username)) {
    queue_.push(
        // 444 User not logged in (cant find username)
        std::make_pair(fd, numeric_reply_(444, fd, message[1])));
    return;
  }
  int user_fd = map_name_fd_[username];
  if (clients_[user_fd].get_server_operator_status() == 1) {
    // that user is already an operator
    return;
  }
  if (message[2] == operator_password_) {
    // 381 You are now an IRC operator
    clients_[user_fd].set_server_operator_status(1);
    queue_.push(std::make_pair(fd, numeric_reply_(381, fd, "")));
  } else {
    // 464 password incorrect
    queue_.push(std::make_pair(fd, numeric_reply_(464, fd, "")));
  }
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

}  // namespace irc