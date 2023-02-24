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

void Server::mode_user_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  std::string nick = client.get_nickname();
  if (message.size() == 2) {
    // 221 answer a query about clients's own modes
    queue_.push(
        std::make_pair(fd, numeric_reply_(221, fd, client.get_usermodes_())));
    return;
  }
  // only the server operator can change modes, otherwise command silently
  // ignored
  if (!client.get_server_operator_status()) {
#if DEBUG
    std::cout << "you are not operator!";
#endif
    return;
  }
  std::string flags = message[2];
  if (message.size() > 3) {
    // 401 unknown command
    queue_.push(std::make_pair(fd, numeric_reply_(421, fd, message[3])));
  }
  bool sign = true;
  bool badflag = false;
  std::map<bool, std::string> map_sign_flag;  // store the changed values as +/-
                                              // to be printed out at the end
  std::vector<char> addedflags;
  std::vector<char> removedflags;
  // check for valid flags, adding them to the list or throwing errors
  for (size_t i = 0; i < flags.size(); ++i) {
    char current = flags.at(i);
    if (current == '+') {
      sign = true;
    } else if (current == '-') {
      sign = false;
    }
    // handle -o
    else if (current == 'o' && !sign && client.get_server_operator_status()) {
      client.set_server_operator_status(0);
      removedflags.push_back('o');
    }
    // ignore +o
    else if (current == 'o' && sign == true) {
      continue;
    }
    // handle +s
    else if (current == 's' && sign && !client.get_server_notices_status()) {
      client.set_server_notices_status(1);
      addedflags.push_back('s');
    }
    // handle -s
    else if (current == 's' && !sign && client.get_server_notices_status()) {
      client.set_server_notices_status(0);
      removedflags.push_back('s');
    }
    // setting the bool, it can be set more than once but error msg will only be
    // printed once
    else if (current != 's' && current != 'o' && current != '+' &&
             current != '-') {
      badflag = true;
    }
  }
  if (badflag) {
    // 501 err_ unknownmode
    queue_.push(std::make_pair(fd, numeric_reply_(501, fd, message[2])));
  }
  if (addedflags.empty() && removedflags.empty()) {
    return;
  }
  std::string flags_changed = "irc: MODE " + nick + " ";

  if (!removedflags.empty()) {
    flags_changed += " -";
    for (size_t i = 0; i < removedflags.size(); ++i) {
      flags_changed += removedflags.at(i);
    }
  }
  if (!addedflags.empty()) {
    flags_changed += " +";
    for (size_t i = 0; i < addedflags.size(); ++i) {
      flags_changed += addedflags.at(i);
    }
  }
  queue_.push(std::make_pair(fd, flags_changed));
}

void Server::mode_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  // this error is wrong
  if (message.size() < 2) {
    // Error 461: Not enough parameters
    queue_.push(
        std::make_pair(fd, numeric_reply_(461, fd, client.get_nickname())));
    return;
  }
  if (message[1].at(0) == '#') {
    // is channel mode
    if (!channels_.count(message[1])) {
      // 401 no such channel
      queue_.push(std::make_pair(fd, numeric_reply_(403, fd, message[1])));
      return;
    } else {
      mode_channel_(fd, message, channels_[message[1]]);
    }
  } else {
    // is user mode
    std::string nick = client.get_nickname();
    if (!irc_stringissame(nick, message[1])) {
      if (search_nick_list_(message[1]) == 0) {
        // 401 no such nickname
        queue_.push(std::make_pair(fd, numeric_reply_(401, fd, message[2])));
        return;
      } else {
        // 502 can't change mode for other users
        queue_.push(std::make_pair(fd, numeric_reply_(502, fd, nick)));
        return;
      }
    }
    mode_user_(fd, message);
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
    // 481,"Permission Denied- You're not an IRC operator"
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
  // queue_.push(std::make_pair(victimfd, numeric_reply_(361, "")));
  std::vector<std::string> quitmessage(1, "QUIT");
  quitmessage.push_back("Killed(" + client.get_nickname() + "(" + message[2] +
                        "))");
  quit_(victimfd, quitmessage);
}

}  // namespace irc