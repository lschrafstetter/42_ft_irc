#include "Client.hpp"
#include "include.hpp"

namespace irc {

Client::Client()
    : nickname_(),
      username_(),
      authentication_(0),
      //authentication_status_(0),
      channels_(),
      server_operator_status_(0) {}
Client::~Client() {}
Client::Client(Client const &other) { nickname_ = other.nickname_; };
Client &Client::operator=(Client const &rhs) {
  nickname_ = rhs.nickname_;
  return *this;
}

// setters
void Client::set_nickname(std::string nickname) { nickname_ = nickname; }

void Client::set_username(std::string username) { username_ = username; }

void Client::set_authentication_status(bool status) {
  authentication_status_ = status;
}

void Client::add_channel(std::string channel) { channels_.push_back(channel); }

void Client::remove_channel(std::string channel) {
  std::vector<std::string>::iterator it;
  for (it = channels_.begin(); it != channels_.end(); ++it) {
    if (*it == channel) channels_.erase(it);
  }
}

void Client::set_server_operator_status(bool status) {
  server_operator_status_ = status;
}

std::string Client::get_nickname() const { return nickname_; }

std::string Client::get_username() const { return username_; }

bool Client::get_authentication_status() const {
  return ((auth_status & PASS_AUTH) && (auth_status & USER_AUTH) && (auth_status & NICK_AUTH) && (auth_status & PONG_AUTH));
}

std::vector<std::string> Client::get_channels_list() const { return channels_; }

bool Client::get_server_operator_status() const {
  return server_operator_status_;
}

}  // namespace irc