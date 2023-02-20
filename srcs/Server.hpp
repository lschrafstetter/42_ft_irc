#pragma once

#include "Channel.hpp"
#include "Client.hpp"
#include "include.hpp"

namespace irc {

class Server {
 public:
  Server();
  ~Server();

  void init(int port, std::string password);
  void run();

 private:
  // Not used
  Server &operator=(const Server &other);
  Server(const Server &other);

  int port_;
  std::string server_name_;
  std::string password_;
  std::string operator_password_;
  int socket_fd_;
  std::map<int, Client> clients_;
  std::map<std::string, Channel, irc_stringmapcomparator<std::string> >
      channels_;
  std::map<std::string, int, irc_stringmapcomparator<std::string> > map_name_fd_;
  bool running_;
  std::queue<std::pair<int, std::string> > queue_;
  std::vector<std::pair<std::string,
                        void (Server::*)(int, std::vector<std::string> &)> >
      functions_;
  std::map<int, std::string> error_codes_;
  std::set<int> open_ping_responses_;
  std::time_t creation_time_;

  // general helper functions
  void ping_(int fd);

  // run() helpers
  int epoll_fd_;
  std::map<int, std::string> client_buffers_;
  void check_open_ping_responses_();
  void epoll_init_();
  int create_new_client_connection_(int socket_fd_);
  void read_from_client_fd_(int client_fd_);
  void disconnect_client_(int client_fd);
  void process_message_(int fd, std::vector<std::string> &message);
  std::vector<std::string> get_next_message_(std::string &buffer);
  void send_message_(std::pair<int, std::string> &message);

  // Registration functions
  void pass_(int fd, std::vector<std::string> &message);
  void user_(int fd, std::vector<std::string> &message);
  void nick_(int fd, std::vector<std::string> &message);
  void pong_(int fd, std::vector<std::string> &message);

  // PRIVMSG
  void privmsg_(int fd, std::vector<std::string> &message);
  void privmsg_to_channel_(int fd_sender, std::string channelname,
                           std::string message);
  void privmsg_to_user_(int fd_sender, std::string channelname,
                        std::string message);

  // LUSERS
  void lusers_(int fd, std::vector<std::string> &message);
  void lusers_client_op_unknown_(int fd);
  void lusers_channels_(int fd);
  void lusers_me_(int fd);

  // MOTD
  void motd_(int fd, std::vector<std::string> &message);
  void motd_start_(int fd);
  void motd_message_(int fd);
  void motd_end_(int fd);

  // Welcome message
  void welcome_(int fd);

  // Channel functions
  void remove_channel_(int fd, std::vector<std::string> &message);
  void quit_(int fd, std::vector<std::string> &message);
  void join_(int fd, std::vector<std::string> &message);
  void part_(int fd, std::vector<std::string> &message);
  void mode_(int fd, std::vector<std::string> & message);
  void invite_(int fd, std::vector<std::string> &message);
  void kick_(int fd, std::vector<std::string> &message);

  // Server operator functions
  void oper_(int fd, std::vector<std::string> & message);
  void kill_(int fd, std::vector<std::string> & message);
  
  // Helpers
  int search_user_list(std::string user);
  bool search_nick_list(std::string nick);
  std::string numeric_reply_(int error_number, int fd_client,
                             std::string argument);
  bool has_invalid_char_(std::string nick);
  bool validflags_(int fd, std::string flags);
  void send_message_to_channel(const Channel &channel, const std::string &message);

  // Initializers
  void init_error_codes_();
  void init_function_vector_();
};

}  // namespace irc
