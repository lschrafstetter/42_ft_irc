#include "Channel.hpp"

namespace irc {

Channel::Channel()
    : users_(0),
      operators_(0),
      banned_users_(0),
      speakers_(0),
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

const std::vector<std::string>& Channel::get_users(void) const {
  return users_;
}

const std::vector<std::string>& Channel::get_operators(void) const {
  return operators_;
}

const std::vector<std::string>& Channel::get_banned_users(void) const {
  return banned_users_;
}

const std::vector<std::string>& Channel::get_speakers(void) const {
  return speakers_;
}

const std::string& Channel::get_channel_password(void) const {
  return channel_password_;
}

void Channel::set_channel_password(std::string& passw) {
  channel_password_ = passw;
}

const std::string& Channel::get_channel_topic(void) const {
  return channel_topic_;
}

void Channel::set_channel_topic(std::string& topic) { channel_topic_ = topic; }

const size_t& Channel::get_user_limit(void) const { return channel_user_limit_; }

void Channel::set_user_limit(size_t& limit) { channel_user_limit_ = limit; }

bool Channel::is_user(std::string& user_name) const {
  for (size_t i = 0; i < users_.size(); ++i) {
    if (irc_stringissame(user_name, users_[i])) return true;
  }
  return false;
}

bool Channel::is_operator(std::string& user_name) const {
  for (size_t i = 0; i < operators_.size(); ++i) {
    if (irc_stringissame(user_name, operators_[i])) return true;
  }
  return false;
}

bool Channel::is_banned(std::string user_name) const {
  for (size_t i = 0; i < banned_users_.size(); ++i) {
    if (irc_stringissame(user_name, banned_users_[i])) return true;
  }
  return false;
}

bool Channel::is_speaker(std::string& user_name) const {
  for (size_t i = 0; i < speakers_.size(); ++i) {
    if (irc_stringissame(user_name, speakers_[i])) return true;
  }
  return false;
}

void Channel::add_user(std::string& user_name) { users_.push_back(user_name); }

void Channel::add_operator(std::string& user_name) {
  if (!is_operator(user_name)) operators_.push_back(user_name);
}

void Channel::add_banned_user(std::string& user_name) {
  if (!is_banned(user_name)) banned_users_.push_back(user_name);
}

void Channel::add_speaker(std::string& user_name) {
  if (!is_speaker(user_name)) speakers_.push_back(user_name);
}

void Channel::remove_user(std::string& user_name) {
  if (is_operator(user_name)) remove_operator(user_name);
  if (is_speaker(user_name)) remove_speaker(user_name);
  for (std::vector<std::string>::iterator it = users_.begin();
       it != users_.end(); ++it) {
    if (irc_stringissame(user_name, *it)) users_.erase(it);
  }
  if (users_.empty())
#if DEBUG
    std::cout << "Delete Channel" << std::endl;
#endif
  // message "[user_name] PART [channelname]"
}

void Channel::remove_operator(std::string& user_name) {
  for (std::vector<std::string>::iterator it = operators_.begin();
       it != operators_.end(); ++it) {
    if (irc_stringissame(user_name, *it)) operators_.erase(it);
  }
}

void Channel::remove_banned_user(std::string& user_name) {
  for (std::vector<std::string>::iterator it = banned_users_.begin();
       it != banned_users_.end(); ++it) {
    if (irc_stringissame(user_name, *it)) banned_users_.erase(it);
  }
}

void Channel::remove_speaker(std::string& user_name) {
  for (std::vector<std::string>::iterator it = speakers_.begin();
       it != speakers_.end(); ++it) {
    if (irc_stringissame(user_name, *it)) speakers_.erase(it);
  }
}

const std::vector<std::string>& Channel::get_banned_users(void) const {
  return banned_users_;
}

const std::vector<std::string>& Channel::get_speakers(void) const {
  return speakers_;
}

const std::string& Channel::get_channel_password(void) const {
  return channel_password_;
}

void Channel::set_channel_password(std::string& passw) {
  channel_password_ = passw;
}

const std::string& Channel::get_channel_topic(void) const {
  return channel_topic_;
}

void Channel::set_channel_topic(std::string& topic) { channel_topic_ = topic; }

const int& Channel::get_user_limit(void) const { return channel_user_limit_; }

void Channel::set_user_limit(int& limit) { channel_user_limit_ = limit; }

bool Channel::is_user(std::string& user_name) const {
  for (size_t i = 0; i < users_.size(); ++i) {
    if (irc_stringissame(user_name, users_[i])) return true;
  }
  return false;
}

bool Channel::is_operator(std::string& user_name) const {
  for (size_t i = 0; i < operators_.size(); ++i) {
    if (irc_stringissame(user_name, operators_[i])) return true;
  }
  return false;
}

bool Channel::is_banned(std::string& user_name) const {
  for (size_t i = 0; i < banned_users_.size(); ++i) {
    if (irc_stringissame(user_name, banned_users_[i])) return true;
  }
  return false;
}

bool Channel::is_speaker(std::string& user_name) const {
  for (size_t i = 0; i < speakers_.size(); ++i) {
    if (irc_stringissame(user_name, speakers_[i])) return true;
  }
  return false;
}

void Channel::add_user(std::string& user_name) { users_.push_back(user_name); }

void Channel::add_operator(std::string& user_name) {
  if (!is_operator(user_name)) operators_.push_back(user_name);
}

void Channel::add_banned_user(std::string& user_name) {
  if (!is_banned(user_name)) banned_users_.push_back(user_name);
}

void Channel::add_speaker(std::string& user_name) {
  if (!is_speaker(user_name)) speakers_.push_back(user_name);
}

void Channel::remove_user(std::string& user_name) {
  if (is_operator(user_name)) remove_operator(user_name);
  if (is_speaker(user_name)) remove_speaker(user_name);
  for (std::vector<std::string>::iterator it = users_.begin();
       it != users_.end(); ++it) {
    if (irc_stringissame(user_name, *it)) users_.erase(it);
  }
  if (users_.empty())
#if DEBUG
    std::cout << "Delete Channel" << std::endl;
#endif
  // message "[user_name] PART [channelname]"
}

void Channel::remove_operator(std::string& user_name) {
  for (std::vector<std::string>::iterator it = operators_.begin();
       it != operators_.end(); ++it) {
    if (irc_stringissame(user_name, *it)) operators_.erase(it);
  }
}

void Channel::remove_banned_user(std::string& user_name) {
  for (std::vector<std::string>::iterator it = banned_users_.begin();
       it != banned_users_.end(); ++it) {
    if (irc_stringissame(user_name, *it)) banned_users_.erase(it);
  }
}

void Channel::remove_speaker(std::string& user_name) {
  for (std::vector<std::string>::iterator it = speakers_.begin();
       it != speakers_.end(); ++it) {
    if (irc_stringissame(user_name, *it)) speakers_.erase(it);
  }
}

}  // namespace irc