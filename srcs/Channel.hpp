#pragma once

#include "include.hpp"

namespace irc {

enum { C_PRIVATE, C_SECRET, C_INVITE, C_TOPIC, C_OUTSIDE, C_MODERATED };

class Channel {
 public:
  Channel();
  // Channel(std::string& password, std::string& topic, int& user_limit);
  ~Channel();

  void setflag(uint8_t flagname);
  void clearflag(uint8_t flagname);
  bool checkflag(uint8_t flagname) const;
  const std::vector<std::string>& get_users(void) const;
  const std::vector<std::string>& get_operators(void) const;
  const std::vector<std::string>& get_banned_users(void) const;
  const std::vector<std::string>& get_speakers(void) const;
  const std::string& get_channel_password(void) const;
  void  set_channel_password(std::string& passw);
  const std::string& get_channel_topic(void) const;
  void  set_channel_topic(std::string& topic);
  const size_t& get_user_limit(void) const;
  void set_user_limit(size_t& limit);
  bool is_user(std::string& user_name) const;
  bool is_operator(std::string& user_name) const;
  bool is_banned(std::string user_name) const;
  bool is_speaker(std::string& user_name) const;
  void  add_user(std::string& user_name);
  void  add_operator(std::string& user_name);
  void  add_banned_user(std::string& user_name);
  void  add_speaker(std::string& user_name);
  void  remove_user(std::string& user_name);
  void  remove_operator(std::string& user_name);
  void  remove_banned_user(std::string& user_name);
  void  remove_speaker(std::string& user_name);

 private:
  std::vector<std::string> users_;
  std::vector<std::string> operators_;
  std::vector<std::string> banned_users_;
  std::vector<std::string> speakers_;
  std::string channel_password_;
  std::string channel_topic_;
  size_t  channel_user_limit_;
  uint8_t channel_flags_;

  // bools with bits enabling?
};

}  // namespace irc