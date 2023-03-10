#include "Server.hpp"

namespace irc {

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
    queue_.push(std::make_pair(fd, numeric_reply_(411, fd, "PRIVMSG")));
    return;
  } else if (message.size() == 2) {
    // Error 412: No text to send
    queue_.push(std::make_pair(fd, numeric_reply_(412, fd, "")));
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

  // If +n flag is set and user is not in the channel or
  // if +m flag is set and user is not an operator (+o) or speaker (+v)
  if ((channel.checkflag(C_OUTSIDE) && !channel.is_user(clientname)) ||
      (channel.checkflag(C_MODERATED) && !channel.is_operator(clientname) &&
       !channel.is_speaker(clientname)) ||
      channel.is_banned(clientname, client.get_username(),
                        client.get_hostname())) { // if user is banned
    // Error 404: Cannot send to channel
    queue_.push(
        std::make_pair(fd_sender, numeric_reply_(404, fd_sender, channelname)));
    return;
  }

  const std::vector<std::string> &userlist = channel.get_users();

  for (size_t i = 0; i < userlist.size(); ++i) {
    std::stringstream servermessage;
    std::string username = userlist[i];
    std::string sendername = clients_[fd_sender].get_nickname();
    if (sendername != username) {
      servermessage << ":" << client.get_nickmask() << " PRIVMSG "
                    << channelname << " :" << message;
      queue_.push(std::make_pair(map_name_fd_[username], servermessage.str()));
    }
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
  servermessage << ":" << clients_[fd_sender].get_nickmask() << " PRIVMSG "
                << nickname << " :" << message;
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

  // If +n flag is set and user is not in the channel or
  // if +m flag is set and user is not an operator (+o) or speaker (+v)
  if ((channel.checkflag(C_OUTSIDE) && !channel.is_user(clientname)) ||
      (channel.checkflag(C_MODERATED) && !channel.is_operator(clientname) &&
       !channel.is_speaker(clientname)) ||
      channel.is_banned(clientname, client.get_username(),
                        client.get_hostname()))
    return;

  const std::vector<std::string> &userlist = channel.get_users();

  for (size_t i = 0; i < userlist.size(); ++i) {
    std::stringstream servermessage;
    std::string username = userlist[i];
    std::string sendername = clients_[fd_sender].get_nickname();
    if (sendername != username) {
      servermessage << ":" << client.get_nickmask() << " NOTICE " << channelname
                    << " :" << message;
      queue_.push(std::make_pair(map_name_fd_[username], servermessage.str()));
    }
  }
}

void Server::notice_to_user_(int fd_sender, std::string nickname,
                             std::string message) {
  if (map_name_fd_.find(nickname) == map_name_fd_.end())
    return;

  std::stringstream servermessage;
  servermessage << ":" << clients_[fd_sender].get_nickmask() << " NOTICE "
                << nickname << " :" << message;
  queue_.push(std::make_pair(map_name_fd_[nickname], servermessage.str()));
}

} // namespace irc