#pragma once

#include "include.hpp"
#define PASS_AUTH 0x01 //0b00000001 if (authentication_ & PASS_AUTH) means this bit is a 1
#define USER_AUTH 0x02 //0b00000010 if (authentication_ & USER_AUTH)
#define NICK_AUTH 0x04 //0b00000100
#define PONG_AUTH 0x08 //0b00001000

namespace irc {

class Client {
public:
  Client();
  Client(Client const &other);
  Client &operator=(Client const &rhs);
  ~Client();

  //are the getters and setters actually necessary??
  // setters
  void set_nickname(std::string nickname);
  void set_username(std::string username);
  void set_auth_status(int8_t status);
  void add_channel(std::string channel);
  void remove_channel(std::string channel);
  void set_server_operator_status(bool status);
  void set_ping(bool ping);

  // getters
  std::string get_nickname() const;
  std::string get_username() const;
  bool get_auth_status() const;
  std::vector<std::string> get_channels_list() const;
  bool get_server_operator_status() const;
  bool get_ping_status() const;

private:
  std::string nickname_;
  std::string username_;
  uint8_t auth_status_;
  /* bool authentication_status_;
  bool password_aut_;
  bool nick_auth_;
  bool user_auth_; */
  bool ping_;
  std::vector<std::string> channels_;
  bool server_operator_status_;

};

} // namespace irc