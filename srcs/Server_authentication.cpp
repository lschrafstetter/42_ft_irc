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
        std::make_pair(fd, numeric_reply_(462, fd, "")));
    return;
  }
  if (message.size() == 1) {
    // Error 461: Not enough parameters
    queue_.push(
        std::make_pair(fd, numeric_reply_(461, fd, "PASS")));
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
    queue_.push(std::make_pair(fd, numeric_reply_(464, fd, "")));
  }
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
    queue_.push(std::make_pair(fd, numeric_reply_(464, fd, "")));
    return;
  }
  if (client.get_status(USER_AUTH)) {
    // Error 462: You may not reregister 
    queue_.push(std::make_pair(fd, numeric_reply_(462, fd, "")));
    return;
  }
  if (message.size() < 4) {
    // Error 461: Not enough parameters 
    queue_.push(std::make_pair(fd, numeric_reply_(461, fd, "USER")));
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
  std::cout << "inside nick function\n";
#endif
  if (!client.get_status(PASS_AUTH)) {
    // Error 464: Password incorrect
    queue_.push(
        std::make_pair(fd, numeric_reply_(464, fd, "")));
    return;
  }
  if (message.size() == 1) {
    // Error 461: Not enough parameters
    queue_.push(
        std::make_pair(fd, numeric_reply_(461, fd, "NICK")));
    return;
  }
  if (message[1].size() > 9 || nick_has_invalid_char_(message[1])) {
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

bool Server::nick_has_invalid_char_(std::string nick) {
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

}