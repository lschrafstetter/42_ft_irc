#include "Client.hpp"
#include "Server.hpp"

namespace irc {

// called when the user enters PASS
void Server::authenticate_password(std::vector<std::string> message_content,
                                   Client client) {
  if (client.get_authentication_status() == 1) {
    std::cout << "Password already authenticated\n";
    return;
  }
  if (message_content.size() == 1 && message_content[0] == password_) {
    client.set_authentication_status(1);
    std::cout << "Password authenticated; access permitted\n";
  } else {
    std::cout << "Password incorrect; access denied\n";
  }
}

void Server::set_username(std::string username, Client client) {
	client.set_username(username);
}

void Server::set_nickname(std::string nickname, Client client) {
	client.set_nickname(nickname);
}

void Server::remove_channel(std::string channel, Client client) {
	client.remove_channel(channel);
}

void Server::try_create_operator(std::string password, Client client) {
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

void Server::remove_operator(std::string password, Client client) {
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
}

std::string Server::get_nickname( Client client ) const {
	return client.get_nickname();
}

std::string Server::get_username( Client client ) const {
	return client.get_username();
}

bool Server::get_authentication_status( Client client ) const {
	return client.get_authentication_status();
}

std::vector<std::string> Server::get_channels_list( Client client ) const {
	return client.get_channels_list();
}

bool Server::get_server_operator_status( Client client ) const {
	return client.get_server_operator_status();
}

} // namespace irc