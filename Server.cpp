#include "include.hpp"
#include "Server.hpp"

namespace irc {

Server::Server(int port, std::string password) {
  port_ = port;
  password_ = password;
}

Server::~Server() {}

// Not used
Server::Server() {}
Server &Server::operator=(const Server &other) { 
  (void) other;
  return *this; }
Server::Server(const Server &other) {
  (void) other;
}

}  // namespace irc