#include "Channel.hpp"

namespace irc {

Channel::Channel()
    : users_(0),
      operators_(),
      banned_users_(),
      speakers_(),
      invited_users_(),
      channel_password_(),
      channel_topic_(),
      channel_user_limit_(0),
      channel_flags_(0) {}

Channel::Channel(const std::string& creator)
    : users_(1, creator),
      operators_(),
      banned_users_(),
      speakers_(),
      invited_users_(),
      channel_password_(),
      channel_topic_(),
      channel_user_limit_(0),
      channel_flags_(0) {
        operators_.insert(creator);
      }

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

const std::set<std::string, irc_stringmapcomparator<std::string> >& Channel::get_operators(void) const {
  return operators_;
}

const std::set<std::string, irc_stringmapcomparator<std::string> >& Channel::get_banned_users(void) const {
  return banned_users_;
}

const std::set<std::string, irc_stringmapcomparator<std::string> >& Channel::get_speakers(void) const {
  return speakers_;
}

const std::set<std::string, irc_stringmapcomparator<std::string> >& Channel::get_invited_users(void) const {
  return invited_users_;
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

bool Channel::is_user(const std::string& user_name) const {
  for (size_t i = 0; i < users_.size(); ++i) {
    if (irc_stringissame(user_name, users_[i])) return true;
  }
  return false;
}

bool Channel::is_operator(const std::string& user_name) const {
  if (operators_.find(user_name) != operators_.end())
    return true;
  return false;
}

bool Channel::is_banned(const std::string& user_name) const {
  if (banned_users_.find(user_name) != banned_users_.end())
    return true;
  return false;
}

bool Channel::is_speaker(const std::string& user_name) const {
  if (speakers_.find(user_name) != speakers_.end())
    return true;
  return false;
}

bool  Channel::is_invited(const std::string& user_name) const {
  return speakers_.find(user_name) != invited_users_.end();
  // if (speakers_.find(user_name) != invited_users_.end())
  //   return true;
  // return false;
}

void Channel::add_user(const std::string& user_name) { users_.push_back(user_name); }

void Channel::add_operator(const std::string& user_name) {
  operators_.insert(user_name);
}

void Channel::add_banned_user(const std::string& user_name) {
  banned_users_.insert(user_name);
}

void Channel::add_speaker(const std::string& user_name) {
  speakers_.insert(user_name);
}

void  Channel::add_invited_user(const std::string& user_name) {
  invited_users_.insert(user_name);
}

void Channel::remove_user(const std::string& user_name) {
  if (is_operator(user_name)) remove_operator(user_name);
  if (is_speaker(user_name)) remove_speaker(user_name);
  for (std::vector<std::string>::iterator it = users_.begin();
       it != users_.end(); ++it) {
    if (irc_stringissame(user_name, *it)) users_.erase(it);
  }
}

void Channel::remove_operator(const std::string& user_name) {
  operators_.erase(user_name);
  // for (std::vector<std::string>::iterator it = operators_.begin();
  //      it != operators_.end(); ++it) {
  //   if (irc_stringissame(user_name, *it)) operators_.erase(it);
  // }
}

void Channel::remove_banned_user(const std::string& user_name) {
  banned_users_.erase(user_name);
  // for (std::vector<std::string>::iterator it = banned_users_.begin();
  //      it != banned_users_.end(); ++it) {
  //   if (irc_stringissame(user_name, *it)) banned_users_.erase(it);
  // }
}

void Channel::remove_speaker(const std::string& user_name) {
  speakers_.erase(user_name);
  // for (std::vector<std::string>::iterator it = speakers_.begin();
  //      it != speakers_.end(); ++it) {
  //   if (irc_stringissame(user_name, *it)) speakers_.erase(it);
  // }
}

void  Channel::remove_invited_user(const std::string& user_name) {
  invited_users_.erase(user_name);
}

}  // namespace irc