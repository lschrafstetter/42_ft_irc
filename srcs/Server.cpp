#include "include.hpp"
#include "Server.hpp"

namespace irc {

Server::Server() : running_(false) {}

Server::~Server() {
  if (socket_fd_ > 0)
    close(socket_fd_);
}

void Server::init(int port, std::string password) {
  struct sockaddr_in server_addr, client_addr;

  if (running_)
    throw std::runtime_error("Server already running.");

  // Create a socket
  if ((socket_fd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    std::cerr << "Error: could not create socket\n";
    throw std::runtime_error("Could not open socket");
  }

  // Configure the server address structure
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);

  // Bind the socket to the specified port
  if (bind(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) <
      0) {
    close (socket_fd_);
    throw std::runtime_error("Could not bind socket to port");
  }

  // Start listening for incoming connections
  if (listen(socket_fd_, MAX_CLIENTS) < 0) {
    close (socket_fd_);
    throw std::runtime_error("Could not initialize listening on port");
  }

  // Update server state
  running_ = true;

  #if DEBUG
    std::cout << "Server is now listening on port " << port << std::endl;
  #endif
}

void Server::run() {
  if (!running_)
    throw std::runtime_error("Server not running. Canceled trying to run server.");

  int epoll_fd = epoll_create(MAX_CLIENTS);

  if (epoll_fd < 0)
    throw std::runtime_error("Failed to create epoll instance");

  struct epoll_event event;
  std:memset(&event, 0, sizeof(event));
  event.events = EPOLLIN;
  event.data.fd = socket_fd_;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd_, &event) < 0) {
    close(epoll_fd);
    throw std::runtime_error("Failed to add socket file descriptor to epoll list");
  }

  while (1) {
    std::cout << "loop running" << std::endl;
    sleep(1);
  }
}

// Not used
Server &Server::operator=(const Server &other) { 
  (void) other;
  return *this; }
Server::Server(const Server &other) {
  (void) other;
}

}  // namespace irc