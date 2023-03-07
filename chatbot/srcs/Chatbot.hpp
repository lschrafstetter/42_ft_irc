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
};

}  // namespace irc