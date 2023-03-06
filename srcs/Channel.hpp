#pragma once

#include "Client.hpp"
#include "include.hpp"

namespace irc {

enum { C_INVITE, C_TOPIC, C_OUTSIDE, C_MODERATED };

struct topicstatus {
  bool topic_is_set;
  std::string topic;
  std::string topicsetter;
  std::time_t time_of_topic_change;
};

struct banmask {
  std::string banned_nickname;
  std::string banned_username;
  std::string banned_hostname;
  std::string banned_by;
  std::time_t time_of_ban;
};

class Channel {
 public:
  Channel();
  Channel(const std::string& creator, const std::string& name);
  Channel(const Channel& other);
  Channel& operator=(const Channel& other);
  ~Channel();

  // Getters
  void setflag(uint8_t flagname);
  const std::vector<std::string>& get_users(void) const;
  const std::set<std::string, irc_stringmapcomparator<std::string> >&
  get_operators(void) const;
  const std::vector<banmask>& get_banned_users(void) const;
  const std::set<std::string, irc_stringmapcomparator<std::string> >&
  get_speakers(void) const;
  const std::set<std::string, irc_stringmapcomparator<std::string> >&
  get_invited_users(void) const;
  const std::string& get_channel_password(void) const;
  const std::string& get_channel_topic(void) const;
  const size_t& get_user_limit(void) const;
  bool is_user(const std::string& user_name) const;
  bool is_operator(const std::string& user_name) const;
  bool is_banned(const std::string& nickname, const std::string& username,
                 const std::string& hostname) const;
  bool is_speaker(const std::string& user_name) const;
  bool is_invited(const std::string& user_name) const;
  bool is_topic_set() const;
  size_t get_topic_set_time() const;
  const std::string& get_topic_setter_name() const;
  const std::string& get_topic_name() const;
  const std::string& get_channelname() const;
  std::time_t get_creationtime();

  // Setters
  void clearflag(uint8_t flagname);
  bool checkflag(uint8_t flagname) const;
  void set_channel_password(std::string& passw);
  void set_channel_topic(std::string& topic);
  void set_user_limit(size_t limit);
  void add_user(const std::string& user_name);
  void add_operator(const std::string& user_name);
  bool add_banmask(const std::string& nickname, const std::string& username,
                   const std::string& hostname, const std::string& banned_by);
  void add_speaker(const std::string& user_name);
  void add_invited_user(const std::string& user_name);
  void remove_user(const std::string& user_name);
  void remove_operator(const std::string& user_name);
  std::pair<size_t, std::string> remove_banmask(const std::string &arg);
  void remove_speaker(const std::string& user_name);
  void remove_invited_user(const std::string& user_name);
  void set_topic(const std::string& topic, const std::string& name_of_setter);
  void clear_topic();
  void change_nickname(const std::string& old_nickname, const std::string& new_nickname);

 private:
  std::vector<std::string> users_;
  std::set<std::string, irc_stringmapcomparator<std::string> > operators_;
  std::set<std::string, irc_stringmapcomparator<std::string> > speakers_;
  std::set<std::string, irc_stringmapcomparator<std::string> > invited_users_;
  std::vector<banmask> banned_users_;
  std::string channel_password_;
  std::string channel_topic_;
  std::string channel_name_;
  size_t channel_user_limit_;
  uint8_t channel_flags_;
  topicstatus topicstatus_;
  std::time_t channel_creationtime;
};

}  // namespace irc