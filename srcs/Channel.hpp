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
  uint8_t checkflag(uint8_t flagname);
  const std::vector<std::string>& get_users_(void) const;
  const int&  get_user_limit_(void) const;

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