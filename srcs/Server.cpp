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

  // call reuseport in order to free the port immediately after closing
  int reuse = 1;
  if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse,
                 sizeof(reuse)) < 0) {
    close(socket_fd_);
    throw std::runtime_error("Could not set reuseaddr option");
  }

#ifdef SO_REUSEPORT
  if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEPORT, (const char *)&reuse,
                 sizeof(reuse)) < 0) {
    close(socket_fd_);
    throw std::runtime_error("Could not set reuseport option");
  }
#endif

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

void Server::send_message_to_channel_(const Channel &channel,
                                      const std::string &message) {
  const std::vector<std::string> &userlist = channel.get_users();
  for (size_t i = 0; i < userlist.size(); ++i) {
    queue_.push(std::make_pair(map_name_fd_[userlist[i]], message));
  }
}

void Server::send_message_to_users_with_shared_channels_(Client &client,
                                                         std::string message) {
  std::set<int> fd_users;
  const std::vector<std::string> &channellist = client.get_channels_list();
  for (size_t i = 0; i < channellist.size(); ++i) {
    const std::vector<std::string> &userlist =
        channels_[channellist[i]].get_users();
    for (size_t j = 0; j < userlist.size(); ++j)
      fd_users.insert(map_name_fd_[userlist[j]]);
  }

  std::set<int>::iterator it = fd_users.begin();
  std::set<int>::iterator end = fd_users.end();
  while (it != end) {
    queue_.push(std::make_pair(*(it++), message));
  }
}

void Server::init_function_vector_() {
  functions_.push_back(std::make_pair("PASS", &Server::pass_));
  functions_.push_back(std::make_pair("USER", &Server::user_));
  functions_.push_back(std::make_pair("NICK", &Server::nick_));
  functions_.push_back(std::make_pair("PONG", &Server::pong_));
  functions_.push_back(std::make_pair("PING", &Server::ping_));
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
  functions_unauthorized_.push_back(std::make_pair("QUIT", &Server::quit_));

  mode_functions_.insert(std::make_pair('n', &Server::mode_channel_n_));
  mode_functions_.insert(std::make_pair('o', &Server::mode_channel_o_));
  mode_functions_.insert(std::make_pair('i', &Server::mode_channel_i_));
  mode_functions_.insert(std::make_pair('t', &Server::mode_channel_t_));
  mode_functions_.insert(std::make_pair('m', &Server::mode_channel_m_));
  mode_functions_.insert(std::make_pair('l', &Server::mode_channel_l_));
  mode_functions_.insert(std::make_pair('b', &Server::mode_channel_b_));
  mode_functions_.insert(std::make_pair('v', &Server::mode_channel_v_));
  mode_functions_.insert(std::make_pair('k', &Server::mode_channel_k_));
}

// Not used
Server &Server::operator=(const Server &other) {
  (void)other;
  return *this;
}
Server::Server(const Server &other) { (void)other; }

}  // namespace irc