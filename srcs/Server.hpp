#pragma once

#include "Client.hpp"
#include "Channel.hpp"
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
  bool running_;
  std::queue<std::pair<int, std::string> > queue_;
  std::vector<std::pair<std::string, void (Server::*)(int, std::vector<std::string> &)> > functions_;
  std::map<int, std::string> error_codes_;
  std::set<int> open_ping_responses_;
  std::map<std::string, Channel>  channels_;

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

  //functions which take Client as parameter
  bool search_nick_list(std::string nick);
  void authenticate_password_(int fd, std::vector<std::string> &message);
  void set_username_(int fd, std::vector<std::string> &message);
  void set_nickname_(int fd, std::vector<std::string> &message);
  void remove_channel_(int fd, std::vector<std::string> &message);
  void pong_(int fd, std::vector<std::string> & message);
  // void join_channel_(int fd, std::vector<std::string>& message);
  void init_error_codes_();
  /* void try_create_operator_(int fd, std::vector<std::string> &message);
  void remove_operator_(int fd, std::vector<std::string> &message); */

  //helpers
  std::string numeric_reply_(int numeric, int fd);

};

}  // namespace irc
