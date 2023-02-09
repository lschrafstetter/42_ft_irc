#pragma once

#include "Client.hpp"
#include "include.hpp"

namespace irc {

class Server {
 public:
  Server();
  ~Server();

  void init(int port, std::string password);
  void run();

  //functions which take Client as parameter
  void authenticate_password(std::vector<std::string> message_content, Client client);
  void set_username(std::string username, Client client);
  void set_nickname(std::string nickname, Client client);
  void remove_channel(std::string channel, Client client);
  void try_create_operator(std::string password, Client client);
  void remove_operator(std::string password, Client client);
  std::string get_nickname( Client client ) const;
  std::string get_username( Client client ) const;
  bool get_authentication_status( Client client ) const;
  std::vector<std::string> get_channels_list( Client client ) const;
  bool get_server_operator_status( Client client ) const;



 private:
  int port_;
  std::string password_;
  std::string operator_password_;
  int socket_fd_;
  std::map<int, Client> clients_;
  bool running_;
  std::queue<std::pair<int, std::string> > queue_;

  Server &operator=(const Server &other);
  Server(const Server &other);

  // run() helpers
  int epoll_fd_;
  std::map<int, std::string> client_buffers_;
  void epoll_init_();
  void create_new_client_connection_(int socket_fd_);
  void read_from_client_fd_(int client_fd_);
  void disconnect_client_(int client_fd);
  void process_message_(std::vector<std::string> &message);
  std::vector<std::string> get_next_message_(std::string &buffer);
  void send_message_(std::pair<int, std::string> &message);
};

}  // namespace irc
