#include "include.hpp"
#include "Server.hpp"

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cout << "Usage: ./ircserv [port] [password]" << std::endl;
    return (EXIT_FAILURE);
  }
  int port = std::atoi(argv[1]);
  if (port < 1) {
    std::cout << "Invalid port number" << std::endl;
    return (EXIT_FAILURE);
  }
  std::string password(argv[2]);

  irc::Server server(port, password);

  return 0;





  /* int listen_fd, conn_fd;
  struct sockaddr_in server_addr, client_addr;
  char buffer[2048];

  // Create a socket
  if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    std::cerr << "Error: could not create socket\n";
    return 1;
  }

  // Configure the server address structure
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);

  // Bind the socket to the specified port
  if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) <
      0) {
    std::cerr << "Error: could not bind socket to port " << port << "\n";
    return 1;
  }

  // Start listening for incoming connections
  if (listen(listen_fd, 5) < 0) {
    std::cerr << "Error: could not listen on socket\n";
    return 1;
  }

  std::cout << "Listening on port " << port << "...\n";

  // Wait for a client to connect
  socklen_t client_len = sizeof(client_addr);
  if ((conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr,
                        &client_len)) < 0) {
    std::cerr << "Error: could not accept client connection\n";
    return 1;
  }

  std::cout << "Accepted client connection\n";

  // Receive data from the client
  int n = read(conn_fd, buffer, 2048 - 1);
  if (n < 0) {
    std::cerr << "Error: could not read from socket\n";
    return 1;
  }
  buffer[n] = '\0';
  std::cout << "Received message: " << buffer << "\n";

  // Clean up
  close(conn_fd);
  close(listen_fd); */

}
