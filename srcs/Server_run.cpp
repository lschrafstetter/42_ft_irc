#include "Server.hpp"

namespace irc {

bool running = 1;

static void signalhandler(int signal) {
  (void)signal;
#if DEBUG
  std::cout << "Signalcode: " << signal << std::endl;
#endif
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

  std::cout << "Server is now running. For safe exit, send ^Z (SIGTSTP)"
            << std::endl;

  while (running) {
    check_open_ping_responses_();
    int fds_ready = epoll_wait(epoll_fd_, postbox, MAX_CLIENTS + 1, 100);
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
  std::map<int, Client>::iterator it = clients_.begin();
  std::map<int, Client>::iterator end = clients_.end();
  while (it != end) {
    close(it++->first);
  }
  close(epoll_fd_);
}

void Server::check_open_ping_responses_() {
  std::set<int>::iterator it = open_ping_responses_.begin();
  std::set<int>::iterator end = open_ping_responses_.end();
  while (it != end) {
    Client &client = clients_[*it];
    if (client.get_ping_status())
      open_ping_responses_.erase(it++);
    else if (time(NULL) - client.get_ping_time() > 60) {
#if DEBUG
      std::cout << "Timeout! Disconnecting client " << *it << std::endl;
#endif
      std::stringstream servermessage;
      servermessage << "Error :Closing Link: " << client.get_nickname()
                    << " by " << server_name_;
      if (client.is_authorized()) {
        servermessage << " (Ping timeout)";
      } else {
        servermessage << " (Registration Timeout)";
      }
      queue_.push(std::make_pair(*it, servermessage.str()));
      disconnect_client_(*it++);
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

  // Initialize new client

  char *client_ip = inet_ntoa(client_addr.sin_addr);
  char hostname[NI_MAXHOST];

  if (getnameinfo((struct sockaddr *)&client_addr, client_len, hostname,
                  NI_MAXHOST, NULL, 0, NI_NAMEREQD) != 0) {
    std::cout << "Couldn't resolve hostname of client with fd " << new_client_fd
              << std::endl;
    close(new_client_fd);
    return;
  }

  Client new_client;
  new_client.set_hostname(hostname);
  new_client.set_ip_addr(client_ip);
  clients_.insert(std::make_pair(new_client_fd, new_client));
  std::stringstream registrationprocess;
  registrationprocess
      << "You just connected to " << server_name_ << "!" << std::endl
      << "For authentication please follow this process:" << std::endl
      << "Answer every PING message with the according PONG message"
      << std::endl
      << "Enter the password with: PASS <password>" << std::endl
      << "Register nickname with: NICK <nickname>" << std::endl
      << "Register username with: USER <username> 0 * :<realname>";
  send_message_(std::make_pair(new_client_fd, registrationprocess.str()));
  ping_client_(new_client_fd);
#if DEBUG
  std::cout << "Added new client hostname " << hostname << " and ip "
            << client_ip << std::endl;
#endif
}

void Server::read_from_client_fd_(int client_fd) {
  static char buffer[BUFFERSIZE];

  memset(&buffer, 0, BUFFERSIZE);
  if (read(client_fd, buffer, BUFFERSIZE) < 1) {
    std::vector<std::string> quitmessage(1, "QUIT");
    quitmessage.push_back("EOF from client");
    quit_(client_fd, quitmessage);
    return;
  }
#if DEBUG
  std::cout << "read " << buffer << std::endl;
#endif
  client_buffers_[client_fd] += buffer;
}

void Server::disconnect_client_(int client_fd) {
  client_buffers_.erase(client_fd);
  clients_.erase(client_fd);
  open_ping_responses_.erase(client_fd);
  epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, NULL);
  close(client_fd);
#if DEBUG
  std::cout << "Disconnected client " << client_fd << "!" << std::endl;
#endif
}

void Server::process_message_(int fd, std::vector<std::string> &message) {
  if (clients_[fd].is_authorized()) {
    for (size_t i = 0; i < functions_.size(); ++i) {
      if (irc_stringissame(functions_[i].first, message[0])) {
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
  } else {
    // If not authorized, only PASS, PONG, NICK, USER and QUIT are available
    for (size_t i = 0; i < functions_unauthorized_.size(); ++i) {
      if (irc_stringissame(functions_unauthorized_[i].first, message[0])) {
        (this->*functions_unauthorized_[i].second)(fd, message);
#if DEBUG
        std::cout << "Executing a function " << message[0] << std::endl;
#endif
        return;
      }
    }
#if DEBUG
    std::cout << "Didn't find function " << message[0]
              << " in the vector for functions that are available when you are "
                 "unauthorized"
              << std::endl;
#endif
  }
  return;
}

std::vector<std::string> Server::get_next_message_(std::string &buffer) {
  std::vector<std::string> ret;
  size_t end_of_message = buffer.find("\r\n");

  if (end_of_message == std::string::npos) return ret;

  std::string message = buffer.substr(0, end_of_message);
  buffer.erase(0, end_of_message + 2);

  // Looks for a prefix and discards it
  size_t pos;
  if (message.size() && message.at(0) == ':' &&
      (pos = message.find(" ")) != std::string::npos)
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

void Server::send_message_(std::pair<int, std::string> message) {
  write(message.first, message.second.c_str(), message.second.length());
  write(message.first, "\r\n", 2);
}

void Server::ping_client_(int fd) {
  Client &client = clients_[fd];
  client.set_pingstatus(false);
  client.set_new_ping();
  open_ping_responses_.insert(fd);
  queue_.push(
      std::make_pair(fd, "PING " + client.get_expected_ping_response()));
#if DEBUG
  std::cout << "Sent PING to client with fd " << fd
            << ". Expected response: " << client.get_expected_ping_response()
            << std::endl;
#endif
}

}  // namespace irc