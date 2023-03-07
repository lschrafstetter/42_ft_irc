#include "Chatbot.hpp"

namespace irc {

Chatbot::Chatbot() {}
Chatbot::~Chatbot() {}

// Not used
Chatbot::Chatbot(const Chatbot &other) { (void)other; }
Chatbot &Chatbot::operator=(const Chatbot &other) {
  (void) other;
  return *this;
}

}  // namespace irc