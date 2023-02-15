#pragma once

#include "include.hpp"
enum { C_PRIVATE, C_SECRET, C_INVITE, C_TOPIC, C_OUTSIDE };

class Channel {
 public:
  Channel();
  // Channel(std::string& password, std::string& topic, int& user_limit);
  ~Channel();

  void setflag(uint8_t flagname);
  void clearflag(uint8_t flagname);
  bool checkflag(uint8_t flagname) const;
  const std::vector<std::string>& get_users_(void) const;
  const std::vector<std::string>& get_operators_(void) const;
  const std::vector<std::string>& get_banned_users_(void) const;
  const std::vector<std::string>& get_muted_users_(void) const;
  const std::string& get_channel_password_(void) const;
  const std::string& get_channel_topic_(void) const;
  const int&  get_user_limit_(void) const;
  bool  is_user(std::string user_name) const;
  bool  is_operator(std::string user_name) const;
  bool  is_banned(std::string user_name) const;
  bool  is_muted(std::string user_name) const;

 private:
  // std::vector<std::pair<std::string, uint8_t> >	flags_;
  std::vector<std::string>  users_;
  std::vector<std::string>  operators_;
  std::vector<std::string>  banned_users_;
  std::vector<std::string>  muted_users_;
  std::string               channel_password_;
  std::string               channel_topic_;
  int                       channel_user_limit_;
  uint8_t                   channel_flags_;

  // bools with bits enabling?
};