#include "Chatbot.hpp"

namespace irc {

void Chatbot::init(const std::string &ip, int port) {
  int socket_fd;
  struct sockaddr_in server_addr;

  if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    throw std::runtime_error("socket() error");

  std::memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);

  if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) < 0) {
    close(socket_fd);
    throw std::runtime_error("inet_pton() error");
  }

  if (connect(socket_fd, (const sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    close(socket_fd);
    throw std::runtime_error("connect() error");
  }

  fd_socket_ = socket_fd;
  connected_ = true;
  std::cout << "Successfully connected to server!" << std::endl;
}

}  // namespace irc