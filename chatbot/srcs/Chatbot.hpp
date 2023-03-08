#pragma once

#include "include.hpp"

namespace irc {

class Chatbot {
 public:
  Chatbot();
  ~Chatbot();

  void init(const std::string &ip, int port);
  void run(const std::string &password);
  
 private:
  // Not used
  Chatbot(const Chatbot &other);
  Chatbot &operator=(const Chatbot &other);

  bool connected_;
  bool authenticated_;
  int fd_socket_;
  int fd_epoll_;
  int instance_;
  std::time_t last_auth_try_;
  std::queue<std::string> queue_;

  void epoll_init_();
  void infinite_loop_(const std::string &password);
  void send_message_(const std::string &message);
  std::vector<std::string> get_next_message_(std::string & buffer);
  void process_message_(const std::vector<std::string> &message);
  void send_authentication_request_(const std::string &password);
  std::string extract_nick_(std::string nickmask);
  std::string tell_random_joke_() const;

};

}  // namespace irc