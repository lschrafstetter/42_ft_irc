#include "Server.hpp"

namespace irc {

bool Server::search_nick_list_(const std::string &nick) const {
  std::map<int, Client>::const_iterator it;
  for (it = clients_.begin(); it != clients_.end(); ++it) {
    if (irc_stringissame(nick, (*it).second.get_nickname())) {
      return 1;
    }
  }
  return 0;
}

int Server::search_user_list_(const std::string &user) const {
  std::map<int, Client>::const_iterator it;
  for (it = clients_.begin(); it != clients_.end(); ++it) {
    if (irc_stringissame(user, (*it).second.get_username())) {
      return (*it).first;
    }
  }
  return -1;
}

} // namespace irc