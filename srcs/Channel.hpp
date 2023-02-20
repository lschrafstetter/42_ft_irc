#pragma once

#include "include.hpp"

namespace irc {

enum { C_PRIVATE, C_SECRET, C_INVITE, C_TOPIC, C_OUTSIDE, C_MODERATED };

class Channel {
  public:
    Channel(const std::string& creator);
    ~Channel();

    void setflag(uint8_t flagname);
    void clearflag(uint8_t flagname);
    bool checkflag(uint8_t flagname) const;
    const std::vector<std::string>& get_users(void) const;
    const std::set<std::string>& get_operators(void) const;
    const std::set<std::string>& get_banned_users(void) const;
    const std::set<std::string>& get_speakers(void) const;
    const std::set<std::string>& get_invited_users(void) const;
    const std::string& get_channel_password(void) const;
    void  set_channel_password(std::string& passw);
    const std::string& get_channel_topic(void) const;
    void  set_channel_topic(std::string& topic);
    const size_t& get_user_limit(void) const;
    void  set_user_limit(size_t& limit);
    bool  is_user(const std::string& user_name) const;
    bool  is_operator(const std::string& user_name) const;
    bool  is_banned(const std::string& user_name) const;
    bool  is_speaker(const std::string& user_name) const;
    bool  is_invited(const std::string& user_name) const;
    void  add_user(const std::string& user_name);
    void  add_operator(const std::string& user_name);
    void  add_banned_user(const std::string& user_name);
    void  add_speaker(const std::string& user_name);
    void  add_invited_user(const std::string& user_name);
    void  remove_user(const std::string& user_name);
    void  remove_operator(const std::string& user_name);
    void  remove_banned_user(const std::string& user_name);
    void  remove_speaker(const std::string& user_name);
    void  remove_invited_user(const std::string& user_name);

  private:
    std::vector<std::string> users_;
    std::set<std::string> operators_;
    std::set<std::string> banned_users_;
    std::set<std::string> speakers_;
    std::set<std::string> invited_users_;
    std::string channel_password_;
    std::string channel_topic_;
    size_t  channel_user_limit_;
    uint8_t channel_flags_;

};

}  // namespace irc