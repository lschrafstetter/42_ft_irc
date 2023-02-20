#include "Server.hpp"

namespace irc {

bool running = 1;

static void signalhandler(int signal) {
  (void) signal;
  running = 0;
}

void Server::run() {
  if (!running_)
    throw std::runtime_error(
        "Server not running. Canceled trying to run server.");

  epoll_init_();
  signal(SIGTSTP, signalhandler);

  struct epoll_event postbox[MAX_CLIENTS + 1];
  std::vector<std::string> message;

  while (running) {
    check_open_ping_responses_();
    int fds_ready = epoll_wait(epoll_fd_, postbox, MAX_CLIENTS + 1, 2000);
    if (fds_ready > 0) {
      for (int i = 0; i < fds_ready; i++) {
        if (postbox[i].data.fd == socket_fd_) {
          try {
            int new_client_fd = create_new_client_connection_(postbox[i].data.fd);
            clients_.insert(std::make_pair(new_client_fd, Client()));
            ping_(new_client_fd);
          } catch (std::exception &e) {
#if DEBUG
            std::cout << "Failed to add client connection" << std::endl;
#endif
          }
        } else {
          read_from_client_fd_(postbox[i].data.fd);
          message = get_next_message_(client_buffers_[postbox[i].data.fd]);
          while (!message.empty()) {
            process_message_(postbox[i].data.fd, message);
            message = get_next_message_(client_buffers_[postbox[i].data.fd]);
          }
        }
      }
    }
    while (!queue_.empty()) {
      send_message_(queue_.front());
      queue_.pop();
    }
  }
}

void Server::check_open_ping_responses_() {
  std::set<int>::iterator it = open_ping_responses_.begin();
  std::set<int>::iterator end = open_ping_responses_.end();
  while (it != end) {
    Client &client = clients_[*it];
    if (client.get_ping_status())
      open_ping_responses_.erase(it++);
    else if (time(NULL) - client.get_ping_time() > 100) {
      #ifdef DEBUG
      std::cout << "Timeout! Disconnecting client " << *it << std::endl;
      #endif
      disconnect_client_(*it);
      open_ping_responses_.erase(it++);
    } else 
      ++it;
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

int Server::create_new_client_connection_(int socket_fd_) {
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

  return new_client_fd;

}

void Server::read_from_client_fd_(int client_fd) {
  static char buffer[BUFFERSIZE];

  memset(&buffer, 0, BUFFERSIZE);
  if (read(client_fd, buffer, BUFFERSIZE) == 0) {
    std::vector<std::string> quitmessage(1, "QUIT");
    quitmessage.push_back("EOF from client");
    quit_(client_fd, quitmessage);
    //disconnect_client_(client_fd);
    return;
  }

  client_buffers_[client_fd] += buffer;
}

void Server::disconnect_client_(int client_fd) {
  client_buffers_.erase(client_fd);
  // delete(clients_.find(client_fd));
  std::cout <<"ret value erase " <<clients_.erase(client_fd) <<std::endl;
  // Remove client from all channels, etc.
  epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, NULL);
  close(client_fd);
#if DEBUG
  std::cout << "Disconnected client " << client_fd << "!" << std::endl;
#endif
}

void Server::process_message_(int fd, std::vector<std::string> &message) {
  for (size_t i = 0; i < functions_.size(); ++i) {
    if (functions_[i].first == message[0]) {
      (this->*functions_[i].second)(fd, message);
      #if DEBUG
        std::cout << "Executing a function " << message[0] << std::endl;
      #endif
      return;
    }
  }
  #if DEBUG
    std::cout << "Didn't find function " << message[0] << std::endl;
  #endif
  return;
}

std::vector<std::string> Server::get_next_message_(std::string &buffer) {
  std::vector<std::string> ret;
  size_t end_of_message = buffer.find("\n");

  if (end_of_message == std::string::npos) return ret;

  std::string message = buffer.substr(0, end_of_message);
  buffer.erase(0, end_of_message + 1);

  // Looks for a prefix and discards it
  size_t pos;
  if (message.size() && message.at(0) == ':' && (pos = message.find(" ")) != std::string::npos)
    message.erase(0, pos + 1);

  if (message.size() && message.at(0) == ':') {
    ret.push_back(message.substr(1, message.size() - 1));
    return ret;
  }
  while ((pos = message.find(" ")) != std::string::npos) {
    if (pos > 0) ret.push_back(message.substr(0, pos));
    message.erase(0, pos + 1);
    if (message.size() && message.at(0) == ':') {
      ret.push_back(message.substr(1, message.size() - 1));
      return ret;
    }
  }

  if (!message.empty()) ret.push_back(message);

#if DEBUG
  std::cout << "Parsed next message:";
  for (size_t i = 0; i < ret.size(); ++i) {
    std::cout << " " << ret[i];
  }
  std::cout << std::endl;
#endif

  return ret;
}

void Server::send_message_(std::pair<int, std::string> &message) {
  write(message.first, message.second.c_str(), message.second.length());
  write(message.first, "\r\n", 2);
}

}  // namespace irc