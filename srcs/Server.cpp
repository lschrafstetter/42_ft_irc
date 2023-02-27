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

void Server::send_message_to_channel_(const Channel &channel,
                                      const std::string &message) {
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
  error_codes_.insert(std::make_pair<int, std::string>(221, "")); //<user mode string>
  error_codes_.insert(std::make_pair<int, std::string>(324, "")); //<channel> <mode> <mode params>
  error_codes_.insert(std::make_pair<int, std::string>(329, "")); //rpl creationtime -- maybe these digits are the time it was created?
  error_codes_.insert(std::make_pair<int, std::string>(341, "")); //<nick> <channel> <channel invite>
  error_codes_.insert(
      std::make_pair<int, std::string>(381, ":You are now an IRC operator")); //:
  error_codes_.insert(std::make_pair<int, std::string>(401, ":No such nick")); //<nickname> : No such nick/ channel
  error_codes_.insert(std::make_pair<int, std::string>(402, ":No such server")); //<server name> :No such server
  error_codes_.insert(std::make_pair<int, std::string>(403, ":No such channel")); // <channel name> :No such channel
  error_codes_.insert(
      std::make_pair<int, std::string>(404, ":Cannot send to channel"));//<channel name> :Cannot send to channel
  error_codes_.insert(
      std::make_pair<int, std::string>(405, ":You have joined too many channels"));       // <channel name> :You have joined too many channels
  error_codes_.insert(
      std::make_pair<int, std::string>(411, ":No recipient given")); //:No recipient given (<command>)
  error_codes_.insert(std::make_pair<int, std::string>(412, ":No text to send")); //:No text to send
  error_codes_.insert(std::make_pair<int, std::string>(421, ":Unknown command")); //<command> :Unknown command
  error_codes_.insert(
      std::make_pair<int, std::string>(422, ":MOTD File is missing"));//:MOTD File is missing
  error_codes_.insert(
      std::make_pair<int, std::string>(431, ":No nickame given")); // :No nickname given
  error_codes_.insert(
      std::make_pair<int, std::string>(432, ":Erroneous nickname")); //<nick> :Erroneous nickname
  error_codes_.insert(
      std::make_pair<int, std::string>(433, ":Nickname is already in use")); //<nick> :Nickname is already in use
  error_codes_.insert(
      std::make_pair<int, std::string>(441, ":They aren't on that channel")); //<nick> <channel> :They aren't on that channel
  error_codes_.insert(
      std::make_pair<int, std::string>(442, ":You're not on that channel")); //<channel> :You're not on that channel
  error_codes_.insert(
      std::make_pair<int, std::string>(443, ":is already on channel")); //<user> <channel> :is already on channel
  error_codes_.insert(
      std::make_pair<int, std::string>(444, ":User not logged in")); //<user> :User not logged in
  error_codes_.insert(
      std::make_pair<int, std::string>(451, ":You have not registered")); //:You have not registered
  error_codes_.insert(
      std::make_pair<int, std::string>(461, ":Not enough parameters")); //<command> :Not enough parameters
  error_codes_.insert(
      std::make_pair<int, std::string>(462, ":You may not reregister")); //:You may not reregister
  error_codes_.insert(
      std::make_pair<int, std::string>(464, ":Password incorrect")); //:Password incorrect
  error_codes_.insert(
      std::make_pair<int, std::string>(467, ":Channel key already set")); //<channel> :Channel key already set
  error_codes_.insert(
      std::make_pair<int, std::string>(476, ":Bad channel mask")); // no info; optional message
  error_codes_.insert(
      std::make_pair<int, std::string>(471, ":Cannot join channel (+l)")); //<channel> :Cannot join channel (+1)
  error_codes_.insert(
      std::make_pair<int, std::string>(472, ":is unknown mode char to me")); //<char> :is unknown mode char to me
  error_codes_.insert(
      std::make_pair<int, std::string>(473, ":Cannot join channel (+i)")); // <channel> :Cannot join channel (+i)
  error_codes_.insert(
      std::make_pair<int, std::string>(474, ":Cannot join channel (+b)")); //<channel> :Cannot join channel (+b)
  error_codes_.insert(
      std::make_pair<int, std::string>(475, ":Cannot join channel (+k)")); //<channel> :Cannot join channel (+k)
error_codes_.insert(
      std::make_pair<int, std::string>(476, "Bad Channel Mask")); //optional message 
  error_codes_.insert(
      std::make_pair<int, std::string>(481, ":Permission Denied- You're not an IRC operator")); //:Permission Denied- You're not an IRC operator
  error_codes_.insert(
      std::make_pair<int, std::string>(482, ":You're not channel operator")); //<channel> :You're not channel operator
  error_codes_.insert(
      std::make_pair<int, std::string>(501, ":Unknown MODE flag")); //:Unkown MODE flag
  error_codes_.insert(std::make_pair<int, std::string>(
      502, ":Can't change mode for other users")); //:Can't change mode for other users
  error_codes_.insert(std::make_pair<int, std::string>(
      525, ":Key is not well-formed"));
}

// Reply functions

void Server::RPL_CMD(const Channel &channel, const std::string &client_nick,
                     const std::string &cmd) {
  const std::string &channel_name = channel.get_channelname();
  std::stringstream servermessage;
  servermessage << ":" << client_nick << " " << cmd << " " << channel_name;
  send_message_to_channel_(channel, servermessage.str());
}

void Server::RPL_NOTOPIC(const std::string &client_nick,
                         const std::string &channel_name, int fd) {
  std::stringstream servermessage;
  servermessage << ":" << server_name_ << " 331 " << client_nick << " "
                << channel_name << " :No topic is set";
  queue_.push(std::make_pair(fd, servermessage.str()));
}

void Server::RPL_TOPIC(const Channel &channel, const std::string &client_nick,
                       int fd) {
  const std::string &topic = channel.get_topic_name();
  const std::string &channel_name = channel.get_channelname();
  std::stringstream servermessage;
  servermessage << ":" << server_name_ << " 332 " << client_nick << " "
                << channel_name << " :" << topic;
  queue_.push(std::make_pair(fd, servermessage.str()));
}

void Server::RPL_TOPICWHOTIME(const Channel &channel,
                              const std::string &client_nick, int fd) {
  const std::string &channel_name = channel.get_channelname();
  std::stringstream servermessage;
  servermessage << server_name_ << " 333 " << client_nick << " " << channel_name
                << " " << channel.get_topic_setter_name() << " "
                << channel.get_topic_set_time();
  queue_.push(std::make_pair(fd, servermessage.str()));
}

void Server::RPL_NAMREPLY(const Channel &channel,
                          const std::string &client_nick, int fd) {
  const std::vector<std::string> &user_list = channel.get_users();
  const std::set<std::string, irc_stringmapcomparator<std::string> > &op_list =
      channel.get_operators();
  const std::string &channel_name = channel.get_channelname();
  std::stringstream servermessage;
  servermessage << ":" << server_name_ << " 353 " << client_nick << " = "
                << channel_name << " :";
  for (size_t i = 0; i < user_list.size(); ++i) {
    const std::string &name = user_list[i];
    if (op_list.find(name) != op_list.end()) servermessage << "@";
    servermessage << name << " ";
  }
  queue_.push(std::make_pair(fd, servermessage.str()));
}

void Server::RPL_ENDOFNAMES(const std::string &client_nick,
                            const std::string &channel_name, int fd) {
  std::stringstream servermessage;
  servermessage << ":" << server_name_ << " 366 " << client_nick << " "
                << channel_name << " :End of /NAMES List";
  queue_.push(std::make_pair(fd, servermessage.str()));
}

// Not used
Server &Server::operator=(const Server &other) {
  (void)other;
  return *this;
}
Server::Server(const Server &other) { (void)other; }

}  // namespace irc