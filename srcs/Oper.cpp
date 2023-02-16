#include "Server.hpp"
#include "Client.hpp"

namespace irc {

void Server::oper_(int fd, std::vector<std::string> & message) {
	Client &client = clients_[fd];
	if (message.size() < 3) {
	// Error 461: Not enough parameters
    queue_.push(
        std::make_pair(fd, numeric_reply_(461, fd, client.get_nickname())));
    return;
	}
	int user_fd = search_user_list(message[1]);
	if (user_fd < 0) {
	queue_.push(
		//444 User not logged in (cant find username)
        std::make_pair(fd, numeric_reply_(444, fd, message[1])));
		return ;
	}
	else if (clients_[user_fd].get_server_operator_status() == 1) {
		//that user is already an operator
		return ;
	}
	if (message[2] == operator_password_) {
		//381 You are now an IRC operator
		queue_.push(std::make_pair(fd, numeric_reply_(381, fd, clients_[user_fd].get_username()))); 
		//if the oper call is successful, a "MODE +o" for the client's nickname is used to the rest of the network 
	}
	else {
		// 464 password incorrect
		queue_.push(std::make_pair(fd, numeric_reply_(464, fd, message[2]))); 
	}
}

}