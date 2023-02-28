#include "Server.hpp"

namespace irc {


void Server::kill_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  if (message.size() < 3) {
    // Error 461: Not enough parameters
    queue_.push(
        std::make_pair(fd, numeric_reply_(461, fd, "KILL")));
    return;
  }
  if (!client.get_server_operator_status()) {
    // 481,"Permission Denied- You're not an IRC operator"
    queue_.push(
        std::make_pair(fd, numeric_reply_(481, fd, "")));
    return;
  }
  if (!map_name_fd_.count(message.at(1))) {
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


/**
 * @brief the user leaves all channels specified in the second parameter
 *
 * @param fd the client's file descriptor
 * @param message message[0] == "PART", message[1] ==
 * "channelname[,channelname]"
 */
void Server::part_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  std::string clientname = client.get_nickname();

  if (message.size() < 2) {
    // Error 461: Not enough parameters
    queue_.push(std::make_pair(fd, numeric_reply_(461, fd, "PART")));
    return;
  }

  std::vector<std::string> channellist = split_string(message[1], ',');

  // Leave every channel on the list individually
  for (size_t i = 0; i < channellist.size(); ++i) {
    std::string &channelname = channellist[i];
    std::map<std::string, Channel,
             irc_stringmapcomparator<std::string> >::iterator it =
        channels_.find(channelname);

    // Does the channel exist?
    if (it == channels_.end()) {
      // Error 403: No such channel
      queue_.push(std::make_pair(fd, numeric_reply_(403, fd, channellist[i])));
      continue;
    }

    Channel &channel = (*it).second;
    const std::vector<std::string> &users_in_channel = channel.get_users();

    // Is client a member of that channel?
    if (std::find(users_in_channel.begin(), users_in_channel.end(),
                  clientname) == users_in_channel.end()) {
      // Error 442: You're not on that channel
      queue_.push(std::make_pair(fd, numeric_reply_(442, fd, channelname)));
      continue;
    }

    // If client is the last one in the channel, delete the channel
    if (channel.get_users().size() == 1) {
      channels_.erase(channelname);
      std::stringstream servermessage;
      servermessage << ":" << clientname << " PART " << channelname;
      queue_.push(std::make_pair(fd, servermessage.str()));
    } else {
      RPL_CMD(channel, clientname, "PART");
      channel.remove_user(clientname);
    }
  }
}

/**
 * @brief fd quits the server and writes an appropriate PRIVMSG to all
 * channels that client has joined. The PRIVMSG is "Quit" if the message is
 * size 1. Otherwise, the last parameter is taken as the PRIVMSG.
 *
 * @param fd file descriptor of the client that is quitting
 * @param message message[0] = "QUIT", further arguments optional. Last argument
 * will be taken as quitting message to all channels
 */
void Server::quit_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  std::vector<std::string> channellist = client.get_channels_list();

  // Build quit message: "PRIVMSG" + "Channel[,Channel,Channel]" + "message"
  std::vector<std::string> quitmessage(1, "PRIVMSG");

  // check if exists ------------------------------------------------------
  if (channellist.size())
    quitmessage.push_back(channellist[0]);
  for (size_t i = 1; i < channellist.size(); ++i) {
    quitmessage[1] += ",";
    quitmessage[1] += channellist[i];
  }

  if (message.size() > 1) {
    quitmessage.push_back(message.at(message.size() - 1));
  } else {
    quitmessage.push_back("Quit");
  }

  privmsg_(fd, quitmessage);

  const std::string &clientname = client.get_nickname();
  for (size_t i = 0; i < channellist.size(); ++i) {
    channels_[channellist[i]].remove_user(clientname);
  }
  map_name_fd_.erase(client.get_nickname());
  disconnect_client_(fd);
}



/**
 * @brief he KICK command can be  used  to  forcibly  remove  a  user  from  a
   channel. It 'kicks them out' of the channel (forced PART). Only a channel
 operator may kick another user out of a  channel.
 *
 * @param fd the client's file descriptor
 * @param message message[0] = "KICK", message[1] = <channel>, message[2] =
 <nickname> [, message[3] = <reason>]
 */
void Server::kick_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  const std::string &clientname = client.get_nickname();

  if (message.size() < 3) {
    // Error 461: Not enough parameters
    queue_.push(std::make_pair(fd, numeric_reply_(461, fd, "KICK")));
    return;
  }

  const std::string &channelname = message[1];
  const std::string &victimname = message[2];

  // Is the channelname valid?
  if (!join_valid_channel_name_(channelname)) {
    // Error 476: Bad Channel Mask
    queue_.push(std::make_pair(fd, numeric_reply_(476, fd, channelname)));
    return;
  }

  std::map<std::string, Channel, irc_stringmapcomparator<std::string> >::iterator
      it = channels_.find(channelname);

  // Does the channel exist?
  if (it == channels_.end()) {
    // Error 403: No such channel
    queue_.push(std::make_pair(fd, numeric_reply_(403, fd, channelname)));
    return;
  }

  Channel &channel = (*it).second;

  if (!channel.is_user(clientname)) {
    // Error 442: You're not on that channel
    queue_.push(std::make_pair(fd, numeric_reply_(442, fd, channelname)));
    return;
  }
  if (!channel.is_operator(clientname)) {
    // Error 482: You're not channel operator
    queue_.push(std::make_pair(fd, numeric_reply_(482, fd, channelname)));
    return;
  }
  if (!channel.is_user(victimname)) {
    // Error 441: They aren't on that channel
    queue_.push(std::make_pair(
        fd, numeric_reply_(441, fd, victimname + " " + channelname)));
    return;
  }

  // Send kick message to channel
  std::stringstream servermessage;
  servermessage << ":" << clientname << " KICK " << channelname << " "
                << victimname << " :";
  if (message.size() == 3)
    servermessage << victimname;
  else {
    servermessage << message[3];
    for (size_t i = 4; i < message.size(); ++i) {
      servermessage << " " << message[i];
    }
  }
  send_message_to_channel_(channel, servermessage.str());

  // Finally kick them!
  channel.remove_user(victimname);
  client.remove_channel(channelname);
}

}