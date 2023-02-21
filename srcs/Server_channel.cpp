#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"

namespace irc {

	//invite <nick> <channel>
	void Server::invite_(int fd, std::vector<std::string> &message) {
	Client &client = clients_[fd];
  	if (message.size() < 3) {
      // Error 461: Not enough parameters
      queue_.push(
     std::make_pair(fd, numeric_reply_(461, fd, client.get_nickname())));
      return;
    }
	if (!search_nick_list_(message[1])) {
        // 401 no such nickname
        queue_.push(std::make_pair(fd, numeric_reply_(401, fd, message[2])));
        return;
    } 
	if (client.search_channels(message[2])) {
	//443 is already on channel
		queue_.push(std::make_pair(fd, numeric_reply_(443, fd, message[1] + " " + message[2])));
		return;
	}
	std::map<std::string, Channel, irc_stringmapcomparator<std::string> >::iterator it = channels_.find(message[2]);
	if (it == channels_.end()) {
		//403 no such channel
		queue_.push(std::make_pair(fd, numeric_reply_(403, fd, message[2])));
		return;
	}
	Channel &channel = (*it).second;
	//if channel is mode + i(invite only), the client sending the invite must be a channel operator
	if (channel.checkflag(C_INVITE) && channel.is_operator(client.get_username())) {
		// 482 <channel> You're not channel operator
		queue_.push(std::make_pair(fd, numeric_reply_(482, fd, message[2])));
		return;
	}
	//find the fd of the invited client
    //std::map<std::string, int, irc_stringmapcomparator<std::string> >::iterator itclient = map_name_fd_find(message[1]);
	//client_fd = (*itclient).second;
	int client_fd = map_name_fd_[message[1]];
	//341 <nick> <channel> (channel invite)
	//push the message to the nickname in the invite
	  queue_.push(std::make_pair(client_fd, numeric_reply_(341, fd, message[2] +  " " + message[1])));
	}
}