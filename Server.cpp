#include "include.hpp"

namespace irc {

Server::Server(int port, std::string password) {
  
}

Server::~Server() {}

// Not used
Server::Server() {}
Server::Server &operator=(const Server &other) { return *this; }
Server::Server(const Server &other) {}

}  // namespace irc