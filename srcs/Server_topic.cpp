#include "Server.hpp"

namespace irc {

/**
 * @brief TOPIC command: The TOPIC message is used to change or view the topic
 of a channel. The topic for channel <channel> is returned if there is no
 <topic> given.  If the <topic> parameter is present, the topic for that channel
 will be changed, if the channel modes permit this action.
 *
 * @param fd client's file descriptor
 * @param message message[0] = "TOPIC", message[1] = <channel> [, message[2] =
 <topic>]
 */
void Server::topic_(int fd, std::vector<std::string> &message) {
  // Client &client = clients_[fd];
  // const std::string &clientname = client.get_nickname();

  if (message.size() == 1) {
    // Error 461: Not enough parameters
    queue_.push(std::make_pair(fd, numeric_reply_(461, fd, "TOPIC")));
    return;
  }

  std::string &channelname = message[1];
  std::map<std::string, Channel,
           irc_stringmapcomparator<std::string> >::iterator it =
      channels_.find(channelname);

  // Does the channel exist?
  if (it == channels_.end()) {
    // Error 403: No such channel
    queue_.push(std::make_pair(fd, numeric_reply_(403, fd, channelname)));
    return;
  }

  Channel &channel = (*it).second;
  if (message.size() == 2) {
    topic_send_info_(fd, channelname, channel);
  } else {
    std::string topic = message[2];
    for (size_t i = 3; i < message.size(); ++i)
      topic += " " + message[i];
    topic_set_topic_(fd, channelname, channel, topic);
  }
}

/**
 * @brief Helper for topic_. If no topic is provided in the topic_ function
 * call, only information about the topic and the topicsetter is provided
 *
 * @param fd client's file descriptor
 * @param channelname channelname
 * @param channel channel object
 */
void Server::topic_send_info_(int fd, const std::string &channelname,
                              const Channel &channel) {
  const std::string &clientname = clients_[fd].get_nickname();
  std::stringstream prefix;
  prefix << ":" << server_name_ << " ";

  if (channel.is_topic_set()) {
    RPL_TOPIC(channel, clientname, fd);
    RPL_TOPICWHOTIME(channel, clientname, fd);
  } else {
    RPL_NOTOPIC(clientname, channelname, fd);
  }
}

/**
 * @brief Helper for topic_. If a topic is provided in the topic_ function -
 * provided the client has the rights - it is set or ,in case of an empty string
 * as the topic, cleared
 *
 * @param fd client's file descriptor
 * @param channelname channelname
 * @param channel channel object
 * @param topicname topicname
 */
void Server::topic_set_topic_(int fd, const std::string &channelname,
                              Channel &channel, const std::string &topicname) {
  const Client &client = clients_[fd];
  const std::string &clientname = client.get_nickname();
  if (channel.checkflag(C_TOPIC) && !channel.is_operator(clientname)) {
    // Error 482: You're not channel operator
    queue_.push(std::make_pair(fd, numeric_reply_(482, fd, channelname)));
    return;
  }

  if (topicname.empty())
    channel.clear_topic();
  else
    channel.set_topic(topicname, clientname);

  std::stringstream servermessage;
  servermessage << ":" << server_name_ << " 332 " << clientname << " "
                << channelname << " :" << topicname;
  send_message_to_channel_(channel, servermessage.str());
}

}  // namespace irc