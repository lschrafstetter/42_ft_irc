#include "Server.hpp"
#include "include.hpp"

namespace irc {

Server::Server() : running_(false) {}

Server::~Server() {
  if (socket_fd_ > 0) close(socket_fd_);
}

void Server::init(int port, std::string password) {
  struct sockaddr_in server_addr;

  if (running_) throw std::runtime_error("Server already running.");

  password_ = password;

  // Create a socket
  if ((socket_fd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    throw std::runtime_error("Could not open socket");

  // Configure the server address structure
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);

  // Bind the socket to the specified port
  if (bind(socket_fd_, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    close(socket_fd_);
    throw std::runtime_error("Could not bind socket to port");
  }

  // Start listening for incoming connections
  if (listen(socket_fd_, MAX_CLIENTS) < 0) {
    close(socket_fd_);
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
    throw std::runtime_error(
        "Server not running. Canceled trying to run server.");

  int epoll_fd = epoll_create(MAX_CLIENTS);

  if (epoll_fd < 0) throw std::runtime_error("Failed to create epoll instance");

  struct epoll_event event, clientevenstruct;
  struct sockaddr_in client_addr;
  memset(&event, 0, sizeof(event));
  event.events = EPOLLIN;
  event.data.fd = socket_fd_;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd_, &event) < 0) {
    close(epoll_fd);
    throw std::runtime_error(
        "Failed to add socket file descriptor to epoll list");
  }

#if DEBUG
  std::cout << "Beginning infinite loop" << std::endl;
#endif

  struct epoll_event postbox[MAX_CLIENTS + 1];

  while (1) {
    int fds_ready = epoll_wait(epoll_fd, postbox, MAX_CLIENTS + 1, 1000);
    if (fds_ready > 0) {
      // go through all indices of postbox
      for (int i = 0; i < fds_ready; i++) {
        if (postbox[i].data.fd == socket_fd_) {
          std::cout << "creating new client connection" << std::endl;

          socklen_t client_len = sizeof(client_addr);
          int new_client_fd = accept(
              postbox[i].data.fd, (struct sockaddr *)&client_addr, &client_len);

          if (new_client_fd < 0)
            std::cout << "failed to accept client conenction" << std::endl;

          memset(&clientevenstruct, 0, sizeof(event));
          clientevenstruct.events = EPOLLIN;
          clientevenstruct.data.fd = new_client_fd;

          // Add new client fd to epoll api watchlist
          if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_client_fd, &clientevenstruct) < 0) {
            std::cout << "failed to add client to watchlist" << std::endl;
          }

        } else {
          std::cout << "Message from client with fd: " << postbox[i].data.fd << std::endl;
          char buffer[2048];
          memset(&buffer, 0, 2048);
          if (read(postbox[i].data.fd, buffer, 2048) == 0) {
            std::cout << "Client has disconnected" << std::endl;
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, postbox[i].data.fd, NULL);
            close(postbox[i].data.fd);
          }
          std::cout << buffer << std::endl;
          // add buffer to string
          // try to extract command
          // if command, then do sth
        }
      }
    }

    // new request: add client 1
    // struct epoll_event new_event;
    // event.events = POLLwhatever
    // event.fd = accept(socket_fd)
    // epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event.fd, &event)
    sleep(1);
  }
}

// Not used
Server &Server::operator=(const Server &other) {
  (void)other;
  return *this;
}
Server::Server(const Server &other) { (void)other; }

}  // namespace irc