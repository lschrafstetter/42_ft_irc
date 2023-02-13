#include "Client.hpp"
#include "include.hpp"

namespace irc {

Client::Client()
    : nickname_(""),
      username_(""),
      ping_ (0),
      channels_(),
      server_operator_status_(0),
      auth_status_ (0) {
        #if DEBUG
          std::cout <<"Client constructor\n";
        #endif
      }
Client::~Client() {
  #if DEBUG 
    std::cout <<"Client destructor called\n";
  #endif
}
Client::Client(Client const &other) { nickname_ = other.nickname_; };
Client &Client::operator=(Client const &rhs) {
  nickname_ = rhs.nickname_;
  return *this;
}

// setters
void Client::set_nickname(std::string nickname) {
  nickname_ = nickname; }

void Client::set_username(std::string username) { username_ = username; }

void Client::set_auth_status(int8_t status) {
  auth_status_ |= status;
}

void Client::set_ping(bool ping) {
  ping_ = ping;
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

bool Client::is_fully_authorized() const {
  return (auth_status_ == 15);
}

uint8_t Client::get_auth_status(uint8_t flag) const {
  #if DEBUG
    std::cout <<"get auth status flag set to 1: " << auth_status_ <<std::endl;
  #endif
  return (auth_status_ & flag);
}

std::vector<std::string> Client::get_channels_list() const { return channels_; }

bool Client::get_server_operator_status() const {
  return server_operator_status_;
}

bool Client::get_ping_status() const {
  return ping_;
}

}  // namespace irc