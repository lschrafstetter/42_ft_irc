#include "Server.hpp"

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cout << "Usage: ./ircserv [port] [password]" << std::endl;
    return (EXIT_FAILURE);
  }
  int port = std::atoi(argv[1]);
  if (port < 1024) {                                      // system port to 256...
    std::cout << "Invalid port number" << std::endl;
    return (EXIT_FAILURE);
  }
  std::string password(argv[2]);

  if (password.empty()) {
    std::cout << "Usage: ./ircserv [port] [password]" << std::endl;
    std::cout <<"Please enter a valid password.\n";
    return (EXIT_FAILURE);
  }

  irc::Server server;
  try {
    server.init(port, password);
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
    return (EXIT_FAILURE);
  }

  try {
    server.run();
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
    return (EXIT_FAILURE);
  }
  return 0;
}
