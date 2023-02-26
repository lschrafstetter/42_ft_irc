#include "include.hpp"
#include "Server.hpp"

int main(int argc, char** argv) {
  std::cout << "Compare:" << std::endl;
  std::string str1("Hello");
  std::string str2("Hel?o"), str3("He?o"), str4("*"), str5("Hello"), str6("*ll*"), str7("*l?o*"), str8("Hello*");
  std::cout << irc::irc_wildcard_cmp(str1.c_str(), str2.c_str()) << std::endl;
  std::cout << irc::irc_wildcard_cmp(str1.c_str(), str3.c_str()) << std::endl;
  std::cout << irc::irc_wildcard_cmp(str1.c_str(), str4.c_str()) << std::endl;
  std::cout << irc::irc_wildcard_cmp(str1.c_str(), str5.c_str()) << std::endl;
  std::cout << irc::irc_wildcard_cmp(str1.c_str(), str6.c_str()) << std::endl;
  std::cout << irc::irc_wildcard_cmp(str1.c_str(), str7.c_str()) << std::endl;
  std::cout << irc::irc_wildcard_cmp(str1.c_str(), str8.c_str()) << std::endl;
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
