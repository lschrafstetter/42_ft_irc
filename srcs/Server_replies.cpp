#include "Server.hpp"

namespace irc {

void Server::RPL_CHANNELCMD(const Channel &channel, const Client &client,
                     const std::string &cmd) {
  std::stringstream servermessage;
  servermessage << ":" << client.get_nickmask() << " " << cmd << " "
                << channel.get_channelname();
  send_message_to_channel_(channel, servermessage.str());
}

void Server::RPL_NOTOPIC(const std::string &client_nick,
                         const std::string &channel_name, int fd) {
  std::stringstream reply;
  reply << ":" << server_name_ << " 331 " << client_nick << " "
                << channel_name << " :No topic is set";
  queue_.push(std::make_pair(fd, reply.str()));
}

void Server::RPL_TOPIC(const Channel &channel, const std::string &client_nick,
                       int fd) {
  const std::string &topic = channel.get_topic_name();
  const std::string &channel_name = channel.get_channelname();
  std::stringstream reply;
  reply << ":" << server_name_ << " 332 " << client_nick << " "
                << channel_name << " :" << topic;
  queue_.push(std::make_pair(fd, reply.str()));
}

void Server::RPL_TOPICWHOTIME(const Channel &channel,
                              const std::string &client_nick, int fd) {
  const std::string &channel_name = channel.get_channelname();
  std::stringstream reply;
  reply << ":" << server_name_ << " 333 " << client_nick << " " << channel_name
                << " " << channel.get_topic_setter_name() << " "
                << channel.get_topic_set_time();
  queue_.push(std::make_pair(fd, reply.str()));
}

void Server::RPL_NAMREPLY(const Channel &channel,
                          const std::string &client_nick, int fd) {
  const std::vector<std::string> &user_list = channel.get_users();
  const std::set<std::string, irc_stringmapcomparator<std::string> > &op_list =
      channel.get_operators();
  const std::string &channel_name = channel.get_channelname();
  std::stringstream reply;
  reply << ":" << server_name_ << " 353 " << client_nick << " = "
                << channel_name << " :";
  for (size_t i = 0; i < user_list.size(); ++i) {
    const std::string &name = user_list[i];
    if (op_list.find(name) != op_list.end()) reply << "@";
    reply << name << " ";
  }
  queue_.push(std::make_pair(fd, reply.str()));
}

void Server::RPL_ENDOFNAMES(const std::string &client_nick,
                            const std::string &channel_name, int fd) {
  std::stringstream reply;
  reply << ":" << server_name_ << " 366 " << client_nick << " "
                << channel_name << " :End of /NAMES List";
  queue_.push(std::make_pair(fd, reply.str()));
}

void Server::RPL_INVITING(const Channel &channel, const Client &client,
                          const std::string &invitee, int fd) {
  std::stringstream reply;
  reply << server_name_ << " 341 " << client.get_nickname() << " " << invitee
        << " " << channel.get_channelname();
  queue_.push(std::make_pair(fd, reply.str()));
}

}  // namespace irc