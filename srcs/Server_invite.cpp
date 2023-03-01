#include "Channel.hpp"
#include "Client.hpp"
#include "Server.hpp"

namespace irc {

// invite <nick> <channel>
void Server::invite_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  if (message.size() < 3) {
    // Error 461: Not enough parameters
    queue_.push(std::make_pair(fd, numeric_reply_(461, fd, "INVITE")));
    return;
  }
  std::string invited_name = message[1];
  std::string channel_name = message[2];
  if (!map_name_fd_.count(invited_name)) {
    // 401 no such nickname
    queue_.push(std::make_pair(fd, numeric_reply_(401, fd, channel_name)));
    return;
  }
  std::map<std::string, Channel,
           irc_stringmapcomparator<std::string> >::iterator it =
      channels_.find(channel_name);
  if (it == channels_.end()) {
    // 403 no such channel
    queue_.push(std::make_pair(fd, numeric_reply_(403, fd, channel_name)));
    return;
  }
  Channel &channel = it->second;

  if (channel.is_user(invited_name)) {
    // 443 is already on channel
    queue_.push(std::make_pair(
        fd, numeric_reply_(443, fd, invited_name + " " + channel_name)));
    return;
  }
  // if channel is mode + i(invite only), the client sending the invite must be
  // a channel operator
  if (channel.checkflag(C_INVITE) &&
      !channel.is_operator(client.get_nickname())) {
    // 482 <channel> You're not channel operator
    queue_.push(std::make_pair(fd, numeric_reply_(482, fd, "")));
    return;
  }
  // add the invitee to the invited list of the channel
  channel.add_invited_user(invited_name);
  RPL_INVITING(channel, client, invited_name, fd);
  std::stringstream servermessage;
  servermessage << client.get_nickmask() << " INVITE " << invited_name << " "
                << channel_name;
  queue_.push(std::make_pair(map_name_fd_[invited_name], servermessage.str()));
}

}  // namespace irc