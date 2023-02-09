#include "Server.hpp"

namespace irc {

void Server::run() {
  if (!running_)
    throw std::runtime_error(
        "Server not running. Canceled trying to run server.");

  epoll_init_();

  struct epoll_event postbox[MAX_CLIENTS + 1];
  std::vector<std::string> message;

  while (1) {
    int fds_ready = epoll_wait(epoll_fd_, postbox, MAX_CLIENTS + 1, 2000);
    if (fds_ready > 0) {
      for (int i = 0; i < fds_ready; i++) {
        if (postbox[i].data.fd == socket_fd_) {
          try {
            create_new_client_connection_(postbox[i].data.fd);
          } catch (std::exception &e) {
#if DEBUG
            std::cout << "Failed to add client connection" << std::endl;
#endif
          }
        } else {
          read_from_client_fd_(postbox[i].data.fd);
          message = get_next_message_(client_buffers_[postbox[i].data.fd]);
          while (!message.empty()) {
            process_message_(message);
            message = get_next_message_(client_buffers_[postbox[i].data.fd]);
          }
        }
      }
    }
    while (!queue_.empty()) {
      send_message_(queue_.front());
    }
  }
}

void Server::epoll_init_() {
  epoll_fd_ = epoll_create(MAX_CLIENTS);

  if (epoll_fd_ < 0)
    throw std::runtime_error("Failed to create epoll instance");

#if DEBUG
  std::cout << "Created epoll fd" << std::endl;
#endif

  struct epoll_event event;
  memset(&event, 0, sizeof(event));
  event.events = EPOLLIN;
  event.data.fd = socket_fd_;

  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket_fd_, &event) < 0) {
    close(epoll_fd_);
    throw std::runtime_error(
        "Failed to add socket file descriptor to epoll list");
  }

#if DEBUG
  std::cout << "Added socket to epoll API watchlist" << std::endl;
#endif
}

void Server::create_new_client_connection_(int socket_fd_) {
  struct epoll_event eventstruct;
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);

  int new_client_fd =
      accept(socket_fd_, (struct sockaddr *)&client_addr, &client_len);

  if (new_client_fd < 0)
    std::runtime_error("failed to accept client conenction");

  memset(&eventstruct, 0, sizeof(eventstruct));
  eventstruct.events = EPOLLIN;
  eventstruct.data.fd = new_client_fd;

  // Add new client fd to epoll api watchlist
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, new_client_fd, &eventstruct) < 0) {
    std::runtime_error("failed to add client to watchlist");
  }

#if DEBUG
  std::cout << "Added new client with fd " << new_client_fd << " to watchlist"
            << std::endl;
#endif
}

void Server::read_from_client_fd_(int client_fd) {
  static char buffer[BUFFERSIZE];

  memset(&buffer, 0, BUFFERSIZE);
  if (read(client_fd, buffer, BUFFERSIZE) == 0) {
    disconnect_client_(client_fd);
  }
#if DEBUG
  std::cout << "Added " << buffer << " to buffer of fd " << client_fd
            << std::endl;
#endif

  client_buffers_[client_fd] += buffer;
}

void Server::disconnect_client_(int client_fd) {
  client_buffers_.erase(client_fd);
  clients_.erase(client_fd);
  epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, NULL);
  close(client_fd);
  #if DEBUG
  std::cout << "Disconnected client " << client_fd << "!" 
            << std::endl;
#endif
}

void Server::process_message_(std::vector<std::string> message) {
  (void)message;
  return;
}

std::vector<std::string> Server::get_next_message_(std::string buffer) {
  (void)buffer;
  return std::vector<std::string>();
}

void Server::send_message_(std::pair<int, std::string> message) {
  write(message.first, message.second.c_str(), message.second.length());
}

}  // namespace irc