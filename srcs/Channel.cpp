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

uint8_t Channel::checkflag(uint8_t flagname) {
  return channel_flags_ >> flagname & 1;
}

const std::vector<std::string>& Channel::get_users_(void) const {
  return users_;
}

const int&  Channel::get_user_limit_(void) const {
  return channel_user_limit_;
}

