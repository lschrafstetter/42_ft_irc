#include "Chatbot.hpp"

namespace irc {

Chatbot::Chatbot()
    : connected_(false), authenticated_(false), fd_socket_(-1), fd_epoll_(-1), instance_(0), last_auth_try_(0) {}

Chatbot::~Chatbot() {
  if (fd_socket_ > 0) close(fd_socket_);
  if (fd_epoll_ > 0) close(fd_epoll_);
}

// Not used
Chatbot::Chatbot(const Chatbot &other) { (void)other; }
Chatbot &Chatbot::operator=(const Chatbot &other) {
  (void)other;
  return *this;
}

}  // namespace irc