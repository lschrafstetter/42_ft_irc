#include "Channel.hpp"

namespace irc {

Channel::Channel()
    : users_(0),
      operators_(),
      speakers_(),
      invited_users_(),
      banned_users_(),
      channel_password_(),
      channel_topic_(),
      channel_name_(),
      channel_user_limit_(),
      channel_flags_(0) {
  topicstatus_.topic_is_set = false;
  channel_creationtime = time(NULL);
}

Channel::Channel(const std::string& creator, const std::string& name)
    : users_(1, creator),
      operators_(),
      speakers_(),
      invited_users_(),
      banned_users_(),
      channel_password_(),
      channel_topic_(),
      channel_name_(name),
      channel_user_limit_(),
      channel_flags_(0) {
  operators_.insert(creator);
  topicstatus_.topic_is_set = false;
  channel_creationtime = time(NULL);
}

Channel::Channel(const Channel& other)
    : users_(other.users_),
      operators_(other.operators_),
      speakers_(other.speakers_),
      invited_users_(other.invited_users_),
      banned_users_(other.banned_users_),
      channel_password_(other.channel_password_),
      channel_topic_(other.channel_topic_),
      channel_name_(other.channel_name_),
      channel_user_limit_(other.channel_user_limit_),
      channel_flags_(other.channel_flags_),
      channel_creationtime(other.channel_creationtime) {
  topicstatus_ = other.topicstatus_;
#if DEBUG
  std::cout << "Copy constructor called" << std::endl;
#endif
}

Channel& Channel::operator=(const Channel& other) {
  if (this != &other) {
    users_ = other.users_;
    operators_ = other.operators_;
    banned_users_ = other.banned_users_;
    speakers_ = other.speakers_;
    invited_users_ = other.invited_users_;
    channel_password_ = other.channel_password_;
    channel_topic_ = other.channel_topic_;
    channel_name_ = other.channel_name_;
    channel_user_limit_ = other.channel_user_limit_;
    channel_flags_ = other.channel_flags_;
    channel_creationtime = channel_creationtime;
    topicstatus_ = other.topicstatus_;
  }
#if DEBUG
  std::cout << "assignment operator called" << std::endl;
#endif
  return *this;
}

Channel::~Channel() {
#if DEBUG
  std::cout << "Destructor called" << std::endl;
#endif
}

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

const std::set<std::string, irc_stringmapcomparator<std::string> >&
Channel::get_operators(void) const {
  return operators_;
}

const std::vector<banmask>& Channel::get_banned_users(void) const {
  return banned_users_;
}

const std::set<std::string, irc_stringmapcomparator<std::string> >&
Channel::get_speakers(void) const {
  return speakers_;
}

const std::set<std::string, irc_stringmapcomparator<std::string> >&
Channel::get_invited_users(void) const {
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

const size_t& Channel::get_user_limit(void) const {
  return channel_user_limit_;
}

void Channel::set_user_limit(size_t limit) { channel_user_limit_ = limit; }

bool Channel::is_user(const std::string& user_name) const {
  for (size_t i = 0; i < users_.size(); ++i) {
    if (irc_stringissame(user_name, users_[i])) return true;
  }
  return false;
}

bool Channel::is_operator(const std::string& user_name) const {
  if (operators_.find(user_name) != operators_.end()) return true;
  return false;
}

// banmask: <nickname>!<username>@hostname
bool Channel::is_banned(const std::string& nickname,
                        const std::string& username,
                        const std::string& hostname) const {
  for (size_t i = 0; i < banned_users_.size(); ++i) {
    const banmask& tmp = banned_users_[i];
    if (irc_wildcard_cmp(nickname.c_str(), tmp.banned_nickname.c_str()) &&
        irc_wildcard_cmp(username.c_str(), tmp.banned_username.c_str()) &&
        irc_wildcard_cmp(hostname.c_str(), tmp.banned_hostname.c_str()))
      return true;
  }
  return false;
}

bool Channel::is_speaker(const std::string& user_name) const {
  if (speakers_.find(user_name) != speakers_.end()) return true;
  return false;
}

bool Channel::is_invited(const std::string& user_name) const {
  return invited_users_.find(user_name) != invited_users_.end();
}

void Channel::add_user(const std::string& user_name) {
  users_.push_back(user_name);
}

void Channel::add_operator(const std::string& user_name) {
  operators_.insert(user_name);
}

bool Channel::add_banmask(const std::string& nickname,
                          const std::string& username,
                          const std::string& hostname,
                          const std::string& banned_by) {
  // Banmask already exists?
  for (size_t i = 0; i < banned_users_.size(); ++i) {
    if (banned_users_[i].banned_nickname == nickname &&
        banned_users_[i].banned_username == username &&
        banned_users_[i].banned_hostname == hostname)
      return false;
  }

  banmask new_banmask;
  new_banmask.banned_nickname = nickname;
  new_banmask.banned_username = username;
  new_banmask.banned_hostname = hostname;
  new_banmask.banned_by = banned_by;
  new_banmask.time_of_ban = time(NULL);
  banned_users_.push_back(new_banmask);
  return true;
}

void Channel::add_speaker(const std::string& user_name) {
  speakers_.insert(user_name);
}

void Channel::add_invited_user(const std::string& user_name) {
  invited_users_.insert(user_name);
}

void Channel::remove_user(const std::string& user_name) {
  if (is_operator(user_name)) remove_operator(user_name);
  if (is_invited(user_name)) remove_invited_user(user_name);
  if (is_speaker(user_name)) remove_speaker(user_name);
  for (std::vector<std::string>::iterator it = users_.begin();
       it != users_.end(); ++it) {
    if (irc_stringissame(user_name, *it)) {
      users_.erase(it);
      return;
    }
  }
}

void Channel::remove_operator(const std::string& user_name) {
  operators_.erase(user_name);
}

std::pair<size_t, std::string> Channel::remove_banmask(const std::string& arg) {
  size_t n_removed_masks = 0;
  std::string removed_masks;

  std::string banmask_nickname;
  std::string banmask_username;
  std::string banmask_hostname;
  parse_banmask(arg, banmask_nickname, banmask_username, banmask_hostname);

  std::vector<banmask>::iterator it = banned_users_.begin();
  std::vector<banmask>::iterator end = banned_users_.end();
  while (it != end) {
    banmask& current = *it;
    if (irc_wildcard_cmp(current.banned_nickname.c_str(),
                         banmask_nickname.c_str()) &&
        irc_wildcard_cmp(current.banned_username.c_str(),
                         banmask_username.c_str()) &&
        irc_wildcard_cmp(current.banned_hostname.c_str(),
                         banmask_hostname.c_str())) {
      std::stringstream removed_mask;
      removed_mask << current.banned_nickname << "!" << current.banned_username
                   << "@" << current.banned_hostname;
      if (!removed_masks.empty()) {
        removed_masks += " ";
      }
      removed_masks += removed_mask.str();
      ++n_removed_masks;
      banned_users_.erase(it++);
    } else {
      ++it;
    }
  }
  return std::make_pair(n_removed_masks, removed_masks);
}

void Channel::remove_speaker(const std::string& user_name) {
  speakers_.erase(user_name);
}

void Channel::remove_invited_user(const std::string& user_name) {
  invited_users_.erase(user_name);
}

bool Channel::is_topic_set() const { return topicstatus_.topic_is_set; }

size_t Channel::get_topic_set_time() const {
  // Unix timestamp in seconds
  return topicstatus_.time_of_topic_change;
}

const std::string& Channel::get_topic_setter_name() const {
  return topicstatus_.topicsetter;
}

const std::string& Channel::get_topic_name() const {
  return topicstatus_.topic;
}

void Channel::set_topic(const std::string& topic,
                        const std::string& name_of_setter) {
  topicstatus_.topic_is_set = true;
  topicstatus_.topic = topic;
  topicstatus_.topicsetter = name_of_setter;
  topicstatus_.time_of_topic_change = time(NULL);
}

void Channel::clear_topic() {
  topicstatus_.topic_is_set = false;
  topicstatus_.topic.clear();
}

const std::string& Channel::get_channelname() const { return channel_name_; }

std::time_t Channel::get_creationtime() { return channel_creationtime; }

void Channel::change_nickname(const std::string& old_nickname, const std::string& new_nickname) {
  // Change in userlist
  for (size_t i = 0; i < users_.size(); ++i) {
    if (irc_stringissame(users_[i], old_nickname))
      users_[i] = new_nickname;
  }

  // Change in other lists
  if (operators_.erase(old_nickname))
    operators_.insert(new_nickname);
  if (speakers_.erase(old_nickname))
    speakers_.insert(new_nickname);
  // Invited_users should never contain a member of the channel
}

}  // namespace irc