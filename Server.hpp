#pragma once

#include "include.hpp"
#include "Client.hpp"

namespace irc {

class Server {
 public:
  Server(int port, std::string password);
  ~Server();



 private:
  int port_;
  std::string password_;
  struct pollfd fds_[MAX_CLIENTS + 1];
  std::map<int, Client> clients_;

  Server();
  Server &operator=(const Server &other);
  Server(const Server &other);
};

}  // namespace irc
