#pragma once

#include "include.hpp"
#include "Client.hpp"

namespace irc {

class Server {
 public:
  Server();
  ~Server();

  void init(int port, std::string password);
  void run();

 private:
  int port_;
  std::string password_;
  int socket_fd_;
  std::map<int, Client> clients_;
  bool running_;

  Server &operator=(const Server &other);
  Server(const Server &other);
};

}  // namespace irc
