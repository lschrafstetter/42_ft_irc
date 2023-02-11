#include "Client.hpp"
#include "Server.hpp"

namespace irc {

// called when the user enters PASS
void Server::authenticate_password_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  if (client.get_auth_status() == 1) {
    std::cout << "Password already authenticated\n";
    return;
  }
  if (message.size() == 2 && message[1] == password_) {
    std::cout << "Password authenticated; access permitted\n";
	clients_[fd].set_auth_status(PASS_AUTH);
  } else {
    std::cout << "Password incorrect; access denied\n";
  }
}

void Server::set_username_(int fd, std::vector<std::string> &message) {
  // Check list of usernames!
  // Work in progress
	clients_[fd].set_username(message[1]);
	clients_[fd].set_auth_status(USER_AUTH);
}

void Server::set_nickname_(int fd, std::vector<std::string> &message) {
  /* if (nickname_exists) {
    queue_.push(std::make_pair(fd, "ERR_NICKCOLLISION"));
    return;
  } */
  // Check list of nicknames!
  // Work in progress
	clients_[fd].set_nickname(message[1]);
	clients_[fd].set_auth_status(NICK_AUTH);
}

void Server::answer_ping_(int fd, std::vector<std::string> &message) {
	//check that the digits after the pong are correct
	(void)message;

	clients_[fd].set_ping(0);
	clients_[fd].set_auth_status(PONG_AUTH);
}

void Server::remove_channel_(int fd, std::vector<std::string> &message) {
  // Check validity of message (size, parameters, etc...)
	clients_[fd].remove_channel(message[1]);
}
/* 
void Server::try_create_operator_(int fd, std::vector<std::string> &message) {
	if (client.get_server_operator_status() == 1) {
		return ;
	}
	if (password == operator_password_) {
		client.set_server_operator_status(1);
		std::cout <<"Password correct, " <<client.get_nickname() <<" now has operator status\n";
	}
	else {
		std::cout <<"Password incorrect, operator status cannot be given\n";
	}
}

void Server::remove_operator_(int fd, std::vector<std::string> &message) {
	if (client.get_server_operator_status() == 0) {
		return ;
	}
	if (password == operator_password_) {
		client.set_server_operator_status(0);
		std::cout <<"Operator status removed\n";
	}
	else {
		std::cout <<"Password incorrect, " <<client.get_nickname() <<" still is an operator\n";
	}
} */

} // namespace irc