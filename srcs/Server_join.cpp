#include "Server.hpp"

namespace irc {


void Server::join_(int fd, std::vector<std::string> &message) {
  if (message.size() < 2) {
    // Error 461 :Not enough parameters
    queue_.push(std::make_pair(fd, numeric_reply_(461, fd, "JOIN")));
    return;
  }

  Client &client = clients_[fd];
  const std::string &client_nick = client.get_nickname();
  std::vector<std::string> channel_names = split_string(message[1], ',');
  std::vector<std::string> channel_key;
  if (message.size() > 2)
    channel_key = split_string(message[2], ',');
  size_t key_index = 0;

  for (size_t name_index = 0; name_index < channel_names.size(); ++name_index) {
    const std::string &channel_name = channel_names[name_index];
    if (join_valid_channel_name_(channel_name)) {
      std::map<std::string, Channel, irc_stringmapcomparator<std::string> >::iterator it = channels_.find(channel_name);
      if (it != channels_.end()) {
        Channel &channel = it->second;
        check_priviliges(fd, client, channel, channel_key, key_index);
      } else if (client.get_channels_list().size() >=
                 MAX_CHANNELS) //  user is in too many channels
        // Error 405 :You have joined too many channels
        queue_.push(std::make_pair(fd, numeric_reply_(405, fd, channel_name)));
      else {
        // creating new channel and adding user
        channels_.insert(
            std::make_pair(channel_name, Channel(client_nick, channel_name)));
        client.add_channel(channel_name);
        Channel &channel = channels_.find(channel_name)->second;
        RPL_CHANNELCMD(channel, client, "JOIN");
        RPL_NAMREPLY(channel, client_nick, fd);
        RPL_ENDOFNAMES(client_nick, channel_name, fd);
      }
    } else
      // Error 403 :No such channel
      queue_.push(std::make_pair(fd, numeric_reply_(403, fd, channel_name)));
  }
}

void Server::check_priviliges(int fd, Client &client, Channel &channel,
                              const std::vector<std::string> &channel_key,
                              size_t& key_index) {
  const std::string &client_nick = client.get_nickname();
  const std::string &channel_name = channel.get_channelname();
  size_t key_size = channel_key.size();
  if (channel.is_user(client_nick))  // user is already in channel
    return;
  if (channel.checkflag(C_INVITE) &&
      !(channel.is_invited(client_nick)))  // user is not invited
    // Error 473 :Cannot join channel (+i)
    queue_.push(std::make_pair(fd, numeric_reply_(473, fd, channel_name)));
  else if (channel.is_banned(
               client.get_nickname(), client.get_username(),
               client.get_hostname()))  //  user is banned from channel
    // Error 474 :Cannot join channel (+b)
    queue_.push(std::make_pair(fd, numeric_reply_(474, fd, channel_name)));
  else if (!channel.get_channel_password().empty() && (key_index < key_size || !key_size) &&
           (channel_key.empty() || channel.get_channel_password() !=
               channel_key[key_index++]))  // key_index incrementation test!!!
                                             // // incorrect password
    // Error 475 :Cannot join channel (+k)
    queue_.push(std::make_pair(fd, numeric_reply_(475, fd, channel_name)));
  else if (channel.get_user_limit() > 0 && channel.get_users().size() >=
           channel.get_user_limit())  //  channel userlimit exceeded
    // Error 471 :Cannot join channel (+l)
    queue_.push(std::make_pair(fd, numeric_reply_(471, fd, channel_name)));
  else if (client.get_channels_list().size() >=
           MAX_CHANNELS)  //  user is in too many channels
    // Error 405 :You have joined too many channels
    queue_.push(std::make_pair(fd, numeric_reply_(405, fd, channel_name)));
  else {
    // adding user to existing channel
    channel.add_user(client_nick);
    client.add_channel(channel_name);
    RPL_CHANNELCMD(channel, client, "JOIN");
    if (channel.is_topic_set()) {
      RPL_TOPIC(channel, client_nick, fd);
      RPL_TOPICWHOTIME(channel, client_nick, fd);
    }
    RPL_NAMREPLY(channel, client_nick, fd);
    RPL_ENDOFNAMES(client_nick, channel_name, fd);
  }
}

bool Server::join_valid_channel_name_(const std::string &channel_name) const {
  size_t size = channel_name.size();
  if (size > 1 && size <= 200 &&
      (channel_name.at(0) == '#')) {
    size_t i = 1;
    while (i < size && channel_name.at(i) != ' ' && channel_name.at(i) != ',' &&
           channel_name.at(i) != (char)7 && channel_name.at(i) != '#' &&
           channel_name.at(i) != '&') {
      ++i;
    }
    if (i == size)
      return true;
  }
  return false;
}

}  // namespace irc