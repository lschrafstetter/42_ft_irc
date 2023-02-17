#include "Server.hpp"

namespace irc {

Server::Server() : running_(false) {
  server_name_ = "irc";
  operator_password_ = "garfield";
  init_function_vector_();
  init_error_codes_();
}

Server::~Server() {
  if (socket_fd_ > 0)
    close(socket_fd_);
}

void Server::init(int port, std::string password) {
  struct sockaddr_in server_addr;

  if (running_)
    throw std::runtime_error("Server already running.");

  password_ = password;

  // Create a socket
  if ((socket_fd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    throw std::runtime_error("Could not open socket");

  // Configure the server address structure
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);

  // Bind the socket to the specified port
  if (bind(socket_fd_, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    close(socket_fd_);
    throw std::runtime_error("Could not bind socket to port");
  }

  // Start listening for incoming connections
  if (listen(socket_fd_, MAX_CLIENTS) < 0) {
    close(socket_fd_);
    throw std::runtime_error("Could not initialize listening on port");
  }

  // Update server state
  running_ = true;

#if DEBUG
  std::cout << "Server is now listening on port " << port << std::endl;
#endif
}

void Server::ping_(int fd) {
  Client &client = clients_[fd];
  client.set_pingstatus(false);
  client.set_new_ping();
  open_ping_responses_.insert(fd);
  queue_.push(
      std::make_pair(fd, "PING " + client.get_expected_ping_response()));

#ifdef DEBUG
  std::cout << "Sent PING to client with fd " << fd
            << ". Expected response: " << client.get_expected_ping_response()
            << std::endl;
#endif
}

void Server::init_function_vector_() {
  functions_.push_back(std::make_pair("PASS", &Server::pass_));
  functions_.push_back(std::make_pair("USER", &Server::user_));
  functions_.push_back(std::make_pair("NICK", &Server::nick_));
  functions_.push_back(std::make_pair("PONG", &Server::pong_));
  functions_.push_back(std::make_pair("QUIT", &Server::quit_));
  functions_.push_back(std::make_pair("PRIVMSG", &Server::privmsg_));
  functions_.push_back(std::make_pair("OPER", &Server::oper_));
  functions_.push_back(std::make_pair("MODE", &Server::mode_));
  functions_.push_back(std::make_pair("KILL", &Server::mode_));
}

void Server::init_error_codes_() {
  error_codes_.insert(std::make_pair<int, std::string>(381, "You are now an IRC operator"));
  error_codes_.insert(std::make_pair<int, std::string>(401, "No such nick"));
  error_codes_.insert(std::make_pair<int, std::string>(403, "No such channel"));
  error_codes_.insert(
      std::make_pair<int, std::string>(404, "Cannot send to channel"));
  error_codes_.insert(
      std::make_pair<int, std::string>(411, "No recipient given"));
  error_codes_.insert(std::make_pair<int, std::string>(412, "No text to send"));
  error_codes_.insert(std::make_pair<int, std::string>(421, "Unknown command"));
  error_codes_.insert(
      std::make_pair<int, std::string>(431, "No nickame given"));
  error_codes_.insert(
      std::make_pair<int, std::string>(432, "Erroneous nickname"));
  error_codes_.insert(
      std::make_pair<int, std::string>(433, "Nickname is already in use"));
  error_codes_.insert(
      std::make_pair<int, std::string>(444, "User not logged in"));
  error_codes_.insert(
      std::make_pair<int, std::string>(451, "You have not registered"));
  error_codes_.insert(
      std::make_pair<int, std::string>(461, "Not enough parameters"));
  error_codes_.insert(
      std::make_pair<int, std::string>(462, "You may not reregister"));
  error_codes_.insert(
      std::make_pair<int, std::string>(464, "Password incorrect"));
  error_codes_.insert(
      std::make_pair<int, std::string>(481, "Permission Denied- You're not an IRC operator"));
  error_codes_.insert(
      std::make_pair<int, std::string>(501, "Unknown MODE flag"));
  error_codes_.insert(
      std::make_pair<int, std::string>(502, "Can't change mode for other users"));
}

// Not used
Server &Server::operator=(const Server &other) {
  (void)other;
  return *this;
}
Server::Server(const Server &other) { (void)other; }

} // namespace irc