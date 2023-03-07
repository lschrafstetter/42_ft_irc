#include "Chatbot.hpp"

int main(int argc, char **argv) {
  if (argc != 4) {
    std::cout << "Usage: ./chatbot [ip-address] [port] [password]" << std::endl;
    return (EXIT_FAILURE);
  }
  int port = std::atoi(argv[2]);
  if (port < 1024) {                                      
    std::cout << "Invalid port number" << std::endl;
    return (EXIT_FAILURE);
  }
  std::string password(argv[3]);

  if (password.empty()) {
    std::cout << "Usage: ./ircserv [port] [password]" << std::endl;
    std::cout <<"Please enter a valid password.\n";
    return (EXIT_FAILURE);
  }

  std::string ip(argv[1]);
  irc::Chatbot chatbot;
  try {
    chatbot.init(ip, port);
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
    return (EXIT_FAILURE);
  }

  try {
    chatbot.run(password);
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
    return (EXIT_FAILURE);
  }
  return 0;
}
