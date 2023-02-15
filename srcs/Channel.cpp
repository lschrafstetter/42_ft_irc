#include "Channel.hpp"

Channel::Channel()
    : operators_(0),
      banned_users_(0),
      muted_users_(0),
      channel_password_(""),
      channel_topic_(""),
      channel_user_limit_(0),
      channel_flags_(0) {}

// Channel::Channel(std::string& password, std::string& topic, int& user_limit)
// : 	operators_(0), banned_users_(0), muted_users_(0),
// channel_password_(password), channel_topic_(topic),
// channel_user_limit_(user_limit), channel_flags_(0) {}

Channel::~Channel() {}

void Channel::setflag(uint8_t flagname) { channel_flags_ |= 1 << flagname; }

void Channel::clearflag(uint8_t flagname) {
  channel_flags_ &= ~(1 << flagname);
}

bool Channel::checkflag(uint8_t flagname) const {
  return channel_flags_ >> flagname & 1;
}

const std::vector<std::string>& Channel::get_users_(void) const {
  return users_;
}

const std::vector<std::string>& Channel::get_operators_(void) const {
  return operators_;
}

const std::vector<std::string>& Channel::get_banned_users_(void) const {
  return banned_users_;
}

const std::vector<std::string>& Channel::get_muted_users_(void) const {
  return muted_users_;
}

const std::string& Channel::get_channel_password_(void) const {
  return channel_password_;
}

const std::string& Channel::get_channel_topic_(void) const {
  return channel_topic_;
}

const int&  Channel::get_user_limit_(void) const {
  return channel_user_limit_;
}

bool  Channel::is_user(std::string user_name) const {
  for (std::vector<std::string>::iterator it = users_.begin(); it != users_.end(); ++it) {
    if (user_name == *it)
      return true;
  }
  return false;
}

bool  Channel::is_operator(std::string user_name) const {
  for (std::vector<std::string>::iterator it = operators_.begin(); it != operators_.end(); ++it) {
    if (user_name == *it)
      return true;
  }
  return false;
}

bool  Channel::is_banned(std::string user_name) const {
  for (std::vector<std::string>::iterator it = banned_users_.begin(); it != banned_users_.end(); ++it) {
    if (user_name == *it)
      return true;
  }
  return false;
}

bool  Channel::is_muted(std::string user_name) const {
  for (std::vector<std::string>::iterator it = muted_users_.begin(); it != muted_users_.end(); ++it) {
    if (user_name == *it)
      return true;
  }
  return false;
}
