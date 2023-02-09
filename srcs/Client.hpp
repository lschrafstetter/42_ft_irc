#pragma once

#include "include.hpp"

namespace irc {

class Client {
public:
  Client();
  ~Client();

  // setters
  void set_nickname(std::string nickname);
  void set_username(std::string username);
  void set_authentication_status(bool status);
  void add_channel(std::string channel);
  void remove_channel(std::string channel);
  void set_server_operator_status(bool status);

  // getters
  std::string get_nickname() const;
  std::string get_username() const;
  bool get_authentication_status() const;
  std::vector<std::string> get_channels_list() const;
  bool get_server_operator_status() const;

private:
  std::string nickname_;
  std::string username_;
  bool authentication_status_;
  std::vector<std::string> channels_;
  bool server_operator_status_;

  // not used
  Client(Client const &other);
  Client &operator=(Client const &rhs);
};

} // namespace irc