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
  std::map<std::string, int, irc_stringmapcomparator<std::string> >
      map_name_fd_;
  bool running_;
  std::queue<std::pair<int, std::string> > queue_;
  std::vector<std::pair<std::string,
                        void (Server::*)(int, std::vector<std::string> &)> >
      functions_;
  std::vector<std::pair<std::string,
                        void (Server::*)(int, std::vector<std::string> &)> >
      functions_unauthorized_;
  std::map<int, std::string> error_codes_;
  std::set<int> open_ping_responses_;
  std::time_t creation_time_;
  std::map<char, std::pair<bool, std::string> (Server::*)(
                     int, Channel &, bool, std::vector<std::string>::iterator &,
                     std::vector<std::string>::iterator &)>
      mode_functions_;

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

  // Messages
  void privmsg_(int fd, std::vector<std::string> &message);
  void privmsg_to_channel_(int fd_sender, std::string channelname,
                           std::string message);
  void privmsg_to_user_(int fd_sender, std::string channelname,
                        std::string message);
  void notice_(int fd, std::vector<std::string> &message);
  void notice_to_channel_(int fd_sender, std::string channelname,
                          std::string message);
  void notice_to_user_(int fd_sender, std::string channelname,
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
  void quit_(int fd, std::vector<std::string> &message);
  void join_(int fd, std::vector<std::string> &message);
  void part_(int fd, std::vector<std::string> &message);
  void mode_(int fd, std::vector<std::string> &message);
  void mode_user_();
  void mode_channel_(int fd, std::vector<std::string> &message,
                     Channel &channel);
  void mode_channel_successmessage_(int fd, Channel &channel,
                                   std::vector<char> &added_modes,
                                   std::vector<char> &removed_modes,
                                   std::vector<std::string> &mode_arguments);
  std::pair<bool, std::string> mode_channel_o_(
      int fd, Channel &channel, bool plus,
      std::vector<std::string>::iterator &arg,
      std::vector<std::string>::iterator &end);
  std::pair<bool, std::string> mode_channel_i_(
      int fd, Channel &channel, bool plus,
      std::vector<std::string>::iterator &arg,
      std::vector<std::string>::iterator &end);
  std::pair<bool, std::string> mode_channel_t_(
      int fd, Channel &channel, bool plus,
      std::vector<std::string>::iterator &arg,
      std::vector<std::string>::iterator &end);
  std::pair<bool, std::string> mode_channel_m_(
      int fd, Channel &channel, bool plus,
      std::vector<std::string>::iterator &arg,
      std::vector<std::string>::iterator &end);
  std::pair<bool, std::string> mode_channel_l_(
      int fd, Channel &channel, bool plus,
      std::vector<std::string>::iterator &arg,
      std::vector<std::string>::iterator &end);
  std::pair<bool, std::string> mode_channel_b_(
      int fd, Channel &channel, bool plus,
      std::vector<std::string>::iterator &arg,
      std::vector<std::string>::iterator &end);
  std::pair<bool, std::string> mode_channel_v_(
      int fd, Channel &channel, bool plus,
      std::vector<std::string>::iterator &arg,
      std::vector<std::string>::iterator &end);
  std::pair<bool, std::string> mode_channel_k_(
      int fd, Channel &channel, bool plus,
      std::vector<std::string>::iterator &arg,
      std::vector<std::string>::iterator &end);

  void invite_(int fd, std::vector<std::string> &message);
  void kick_(int fd, std::vector<std::string> &message);

  // Server operator functions
  void oper_(int fd, std::vector<std::string> &message);
  void kill_(int fd, std::vector<std::string> &message);
  void topic_(int fd, std::vector<std::string> &message);
  void topic_send_info_(int fd, const std::string &channelname,
                        const Channel &channel);
  void topic_set_topic_(int fd, const std::string &channelname,
                        Channel &channel, const std::string &topicname);

  // Helpers
  int search_user_list_(const std::string &user) const;
  bool search_nick_list_(const std::string &nick) const;
  std::string numeric_reply_(int error_number, int fd_client,
                             std::string argument);
  bool has_invalid_char_(std::string nick);
  bool validflags_(int fd, std::string flags);
  void send_message_to_channel_(const Channel &channel,
                                const std::string &message);
  void send_RPL_message_(int fd, int RPL_number, const std::string &argument);
  bool valid_channel_name_(const std::string &channel_name) const;

  // Initializers
  void init_error_codes_();
  void init_function_vector_();
};

}  // namespace irc
