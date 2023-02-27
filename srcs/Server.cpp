#include "Server.hpp"

namespace irc {

Server::Server() : running_(false), creation_time_(std::time(NULL)) {
  server_name_ = "ft_irc";
  operator_password_ = "garfield";
  init_function_vector_();
  init_error_codes_();
}

Server::~Server() {
  if (socket_fd_ > 0) close(socket_fd_);
}

void Server::init(int port, std::string password) {
  struct sockaddr_in server_addr;

  if (running_) throw std::runtime_error("Server already running.");

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

void Server::send_message_to_channel_(const Channel &channel, const std::string &message) {
  const std::vector<std::string> &userlist = channel.get_users();
  for (size_t i = 0; i < userlist.size(); ++i) {
    queue_.push(std::make_pair(map_name_fd_[userlist[i]], message));
  }
}


void Server::init_function_vector_() {
  functions_.push_back(std::make_pair("PASS", &Server::pass_));
  functions_.push_back(std::make_pair("USER", &Server::user_));
  functions_.push_back(std::make_pair("NICK", &Server::nick_));
  functions_.push_back(std::make_pair("PONG", &Server::pong_));
  functions_.push_back(std::make_pair("QUIT", &Server::quit_));
  functions_.push_back(std::make_pair("PRIVMSG", &Server::privmsg_));
  functions_.push_back(std::make_pair("LUSERS", &Server::lusers_));
  functions_.push_back(std::make_pair("OPER", &Server::oper_));
  functions_.push_back(std::make_pair("MODE", &Server::mode_));
  functions_.push_back(std::make_pair("KILL", &Server::kill_));
  functions_.push_back(std::make_pair("JOIN", &Server::join_));
  functions_.push_back(std::make_pair("NOTICE", &Server::notice_));
  functions_.push_back(std::make_pair("INVITE", &Server::invite_));
  functions_.push_back(std::make_pair("KICK", &Server::kick_));
  functions_.push_back(std::make_pair("TOPIC", &Server::topic_));
  functions_.push_back(std::make_pair("PART", &Server::part_));


  // Functions that are available when you are unauthorized
  functions_unauthorized_.push_back(std::make_pair("PASS", &Server::pass_));
  functions_unauthorized_.push_back(std::make_pair("USER", &Server::user_));
  functions_unauthorized_.push_back(std::make_pair("NICK", &Server::nick_));
  functions_unauthorized_.push_back(std::make_pair("PONG", &Server::pong_));

  mode_functions_.insert(std::make_pair('o', &Server::mode_channel_o_));
  mode_functions_.insert(std::make_pair('i', &Server::mode_channel_i_));
  mode_functions_.insert(std::make_pair('t', &Server::mode_channel_t_));
  mode_functions_.insert(std::make_pair('m', &Server::mode_channel_m_));
  mode_functions_.insert(std::make_pair('l', &Server::mode_channel_l_));
  mode_functions_.insert(std::make_pair('b', &Server::mode_channel_b_));
  mode_functions_.insert(std::make_pair('v', &Server::mode_channel_v_));
  mode_functions_.insert(std::make_pair('k', &Server::mode_channel_k_));
}

void Server::init_error_codes_() {
  error_codes_.insert(std::make_pair<int, std::string>(221, "")); //print out usermodes
  error_codes_.insert(std::make_pair<int, std::string>(324, "")); //print out channelmodes
  error_codes_.insert(std::make_pair<int, std::string>(329, "")); //rpl creationtime
  error_codes_.insert(
      std::make_pair<int, std::string>(381, "You are now an IRC operator"));
  error_codes_.insert(std::make_pair<int, std::string>(401, "No such nick"));
  error_codes_.insert(std::make_pair<int, std::string>(402, "No such server"));
  error_codes_.insert(std::make_pair<int, std::string>(403, "No such channel"));
  error_codes_.insert(
      std::make_pair<int, std::string>(404, "Cannot send to channel"));
  error_codes_.insert(
      std::make_pair<int, std::string>(405, "You have joined too many channels"));       // pedro
  error_codes_.insert(
      std::make_pair<int, std::string>(411, "No recipient given"));
  error_codes_.insert(std::make_pair<int, std::string>(412, "No text to send"));
  error_codes_.insert(std::make_pair<int, std::string>(421, "Unknown command"));
  error_codes_.insert(
      std::make_pair<int, std::string>(422, ":MOTD File is missing"));
  error_codes_.insert(
      std::make_pair<int, std::string>(431, "No nickame given"));
  error_codes_.insert(
      std::make_pair<int, std::string>(432, "Erroneous nickname"));
  error_codes_.insert(
      std::make_pair<int, std::string>(433, "Nickname is already in use"));
  error_codes_.insert(
      std::make_pair<int, std::string>(441, "They aren't on that channel"));
  error_codes_.insert(
      std::make_pair<int, std::string>(442, "You're not on that channel"));
  error_codes_.insert(
      std::make_pair<int, std::string>(443, "is already on channel"));
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
      std::make_pair<int, std::string>(467, "Channel key already set"));
  error_codes_.insert(
      std::make_pair<int, std::string>(476, "Bad channel mask"));
  error_codes_.insert(
      std::make_pair<int, std::string>(471, "Cannot join channel (+l)"));
  error_codes_.insert(
      std::make_pair<int, std::string>(472, "is unknown mode char to me"));
  error_codes_.insert(
      std::make_pair<int, std::string>(473, "Cannot join channel (+i)"));
  error_codes_.insert(
      std::make_pair<int, std::string>(474, "Cannot join channel (+b)"));
  error_codes_.insert(
      std::make_pair<int, std::string>(475, "Cannot join channel (+k)"));
  error_codes_.insert(
      std::make_pair<int, std::string>(481, "Permission Denied- You're not an IRC operator"));
  error_codes_.insert(
      std::make_pair<int, std::string>(482, "You're not channel operator"));
  error_codes_.insert(
      std::make_pair<int, std::string>(501, "Unknown MODE flag"));
  error_codes_.insert(std::make_pair<int, std::string>(
      502, "Can't change mode for other users"));
  error_codes_.insert(std::make_pair<int, std::string>(
      525, "Key is not well-formed"));
}

// Not used
Server &Server::operator=(const Server &other) {
  (void)other;
  return *this;
}
Server::Server(const Server &other) { (void)other; }

}  // namespace irc