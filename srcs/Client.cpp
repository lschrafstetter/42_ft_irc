#include "Client.hpp"
#include "include.hpp"

namespace irc {

Client::Client()
    : nickname_(""),
      username_(""),
      channels_(),
      server_operator_status_(0),
      server_notices_(0),
      auth_status_(0) {}
Client::~Client() {}

Client::Client(Client const &other) {
  auth_status_ = other.auth_status_;
  nickname_ = other.nickname_;
};

Client &Client::operator=(Client const &rhs) {
  auth_status_ = rhs.auth_status_;
  nickname_ = rhs.nickname_;
  return *this;
}

// setters
void Client::set_nickname(std::string nickname) { nickname_ = nickname; }

void Client::set_username(std::string username) { username_ = username; }

void Client::set_hostname(std::string hostname) { hostname_ = hostname; }

void Client::set_ip_addr(std::string ip_addr) { ip_addr_ = ip_addr; }

void Client::set_status(int8_t status) { auth_status_ |= status; }

void Client::set_pingstatus(bool ping) { pingstatus_.pingstatus = ping; }

void Client::set_new_ping() {
  pingstatus_.time_of_ping = time(NULL);
  std::ostringstream oss;
  oss << pingstatus_.time_of_ping % 42;
  pingstatus_.expected_response = oss.str();
}

void Client::add_channel(std::string channel) { 
  if (std::find(channels_.begin(), channels_.end(), channel) != channels_.end())
    channels_.push_back(channel);
}

void Client::remove_channel(std::string channel) {
  std::vector<std::string>::iterator it;
  for (it = channels_.begin(); it != channels_.end(); ++it) {
    if (*it == channel) channels_.erase(it);
  }
}

void Client::add_invite(std::string invite) { invites_.push_back(invite); }

void Client::remove_invite(std::string invite) {
  std::vector<std::string>::iterator it;
  for (it = invites_.begin(); it != invites_.end(); ++it) {
    if (*it == invite) invites_.erase(it);
  }
}

void Client::set_server_operator_status(bool status) {
  server_operator_status_ = status;
}

void Client::set_server_notices_status(bool status) {
  server_notices_ = status;
}

const std::string &Client::get_nickname() const { return nickname_; }

const std::string &Client::get_username() const { return username_; }

const std::string &Client::get_hostname() const { return hostname_; }

const std::string &Client::get_ip_addr() const { return ip_addr_; }

const std::string &Client::get_nickmask() const {
  std::stringstream nickmask;
  nickmask << get_nickname() << "!" << get_username() << "@" << get_hostname();
  return nickmask.str();
}

bool Client::is_authorized() const { return (auth_status_ == 15); }

bool Client::get_status(uint8_t flag) const { return (auth_status_ & flag); }

const std::vector<std::string> &Client::get_channels_list() const {
  return channels_;
}

const std::vector<std::string> &Client::get_invites_list() const {
  return invites_;
}

bool Client::get_server_operator_status() const {
  return server_operator_status_;
}

bool Client::get_server_notices_status() const { return server_notices_; }

bool Client::get_ping_status() const { return pingstatus_.pingstatus; }

const std::time_t &Client::get_ping_time() const {
  return pingstatus_.time_of_ping;
}

const std::string &Client::get_expected_ping_response() const {
  return pingstatus_.expected_response;
}

void Client::remove_channel_from_channellist(const std::string &channelname) {
  std::vector<std::string>::iterator it =
      std::find(channels_.begin(), channels_.end(), channelname);
  channels_.erase(it);
}

bool Client::search_channels(std::string channel) {
  for (size_t i = 0; i < channels_.size(); i++) {
    if (channel == channels_.at(i)) return true;
  }
  return false;
}

std::string Client::get_usermodes_() {
  std::string ret("");
  if (get_server_operator_status())
    ret += "o";
  if (get_server_notices_status())
    ret += "s";
  return ret;
}


}  // namespace irc