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
  std::map<char, std::pair<size_t, std::string> (Server::*)(
                     int, Channel &, bool, std::vector<std::string>::iterator &,
                     std::vector<std::string>::iterator &)>
      mode_functions_;

  // Server_authentication.cpp
  void pass_(int fd, std::vector<std::string> &message);
  void user_(int fd, std::vector<std::string> &message);
  void nick_(int fd, std::vector<std::string> &message);
  void pong_(int fd, std::vector<std::string> &message);
  void ping_(int fd, std::vector<std::string> &message);
  bool nick_has_invalid_char_(std::string nick);

  // Server_errors.cpp
  std::string numeric_reply_(int error_number, int fd_client,
                             std::string argument);
  void init_error_codes_();

  // Server_invite.cpp
  void invite_(int fd, std::vector<std::string> &message);

  // Server_join.cpp
  void join_(int fd, std::vector<std::string> &message);
  void check_priviliges(int fd, Client &client, Channel &channel,
                        const std::vector<std::string> &channel_key,
                        size_t& key_index);
  bool join_valid_channel_name_(const std::string &channel_name) const;

  // Server_mode.cpp
  void mode_(int fd, std::vector<std::string> &message);
  void mode_user_(int fd, std::vector<std::string> &message);
  void mode_channel_(int fd, std::vector<std::string> &message,
                     Channel &channel);
  void mode_channel_successmessage_(
      int fd, Channel &channel, std::vector<char> &added_modes,
      std::vector<char> &removed_modes,
      std::vector<std::string> &added_mode_arguments,
      std::vector<std::string> &removed_mode_arguments);
  std::pair<size_t, std::string> mode_channel_n_(
      int fd, Channel &channel, bool plus,
      std::vector<std::string>::iterator &arg,
      std::vector<std::string>::iterator &end);
  std::pair<size_t, std::string> mode_channel_o_(
      int fd, Channel &channel, bool plus,
      std::vector<std::string>::iterator &arg,
      std::vector<std::string>::iterator &end);
  std::pair<size_t, std::string> mode_channel_i_(
      int fd, Channel &channel, bool plus,
      std::vector<std::string>::iterator &arg,
      std::vector<std::string>::iterator &end);
  std::pair<size_t, std::string> mode_channel_t_(
      int fd, Channel &channel, bool plus,
      std::vector<std::string>::iterator &arg,
      std::vector<std::string>::iterator &end);
  std::pair<size_t, std::string> mode_channel_m_(
      int fd, Channel &channel, bool plus,
      std::vector<std::string>::iterator &arg,
      std::vector<std::string>::iterator &end);
  std::pair<size_t, std::string> mode_channel_l_(
      int fd, Channel &channel, bool plus,
      std::vector<std::string>::iterator &arg,
      std::vector<std::string>::iterator &end);
  std::pair<size_t, std::string> mode_channel_b_(
      int fd, Channel &channel, bool plus,
      std::vector<std::string>::iterator &arg,
      std::vector<std::string>::iterator &end);
  void mode_channel_b_list_(int fd, const Channel &channel);
  std::pair<size_t, std::string> mode_channel_b_add_banmask_(
      int fd, Channel &channel, std::vector<std::string>::iterator &arg);
  std::pair<size_t, std::string> mode_channel_b_remove_banmask_(
      int fd, Channel &channel, std::vector<std::string>::iterator &arg);
  std::pair<size_t, std::string> mode_channel_v_(
      int fd, Channel &channel, bool plus,
      std::vector<std::string>::iterator &arg,
      std::vector<std::string>::iterator &end);
  std::pair<size_t, std::string> mode_channel_k_(
      int fd, Channel &channel, bool plus,
      std::vector<std::string>::iterator &arg,
      std::vector<std::string>::iterator &end);
  void check_plus_b_no_arg_flag_(int fd, std::vector<std::string> &message,
                                 Channel &channel);
  void mode_print_flags_(int fd, Channel &channel);

  // Server_oper.cpp
  void oper_(int fd, std::vector<std::string> &message);
  int search_user_list_(const std::string &user) const;

  // Server_privmsg.cpp
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

  // Server_quit.cpp
  void kill_(int fd, std::vector<std::string> &message);
  void quit_(int fd, std::vector<std::string> &message);
  void part_(int fd, std::vector<std::string> &message);
  void kick_(int fd, std::vector<std::string> &message);

  // Server_replies.cpp
  void RPL_CHANNELCMD(const Channel &channel, const Client &client,
                      const std::string &cmd);
  void RPL_TOPIC(const Channel &channel, const std::string &client_nick,
                 int fd);
  void RPL_NOTOPIC(const std::string &client_nick,
                   const std::string &channel_name, int fd);
  void RPL_TOPICWHOTIME(const Channel &channel, const std::string &client_nick,
                        int fd);
  void RPL_NAMREPLY(const Channel &channel, const std::string &channel_name,
                    int fd);
  void RPL_ENDOFNAMES(const std::string &client_nick,
                      const std::string &channel_name, int fd);
  void RPL_INVITING(const Channel &channel, const Client &client,
                    const std::string &invitee, int fd);

  // Server_run.cpp helpers
  int epoll_fd_;
  std::map<int, std::string> client_buffers_;
  void check_open_ping_responses_();
  void epoll_init_();
  void create_new_client_connection_(int socket_fd_);
  void read_from_client_fd_(int client_fd_);
  void disconnect_client_(int client_fd);
  void process_message_(int fd, std::vector<std::string> &message);
  std::vector<std::string> get_next_message_(std::string &buffer);
  void send_message_(std::pair<int, std::string> message);
  void ping_client_(int fd);

  // Server_topic.cpp
  void topic_(int fd, std::vector<std::string> &message);
  void topic_send_info_(int fd, const std::string &channelname,
                        const Channel &channel);
  void topic_set_topic_(int fd, const std::string &channelname,
                        Channel &channel, const std::string &topicname);

  // Server_welcome.cpp
  void welcome_(int fd);
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

  // Server.cpp helpers
  void send_message_to_channel_(const Channel &channel,
                                const std::string &message);
  void send_message_to_users_with_shared_channels_(Client &client,
                                                   std::string message);
  void init_function_vector_();
};

}  // namespace irc
