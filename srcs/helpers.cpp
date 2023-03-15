#include "include.hpp"

namespace irc {

std::vector<std::string> split_string(std::string& line, char delim) {
  std::vector<std::string> ret;
  std::string buf;
  std::istringstream temp(line);
  while (std::getline(temp, buf, delim)) ret.push_back(buf);
  return ret;
}

static bool irc_charissame(char a, char b) {
  if (a == b) return true;
  if ((a == '[' || a == '{') && (b == '[' || b == '{'))
    return true;
  else if ((a == ']' || a == '}') && (b == ']' || b == '}'))
    return true;
  else if ((a == '\\' || a == '|') && (b == '\\' || b == '|'))
    return true;
  else if (isupper(a) && islower(b) && a == b - 32)
    return true;
  else if (isupper(b) && islower(a) && b == a - 32)
    return true;
  return false;
}

bool irc_stringissame(const std::string& str1, const std::string& str2) {
  if (str1.size() != str2.size()) return false;
  for (size_t i = 0; i < str1.size(); ++i) {
    if (!irc_charissame(str1.at(i), str2.at(i))) return false;
  }
  return true;
}

bool irc_customlesscomparator(const char* str1, const char* str2) {
  int i = 0;
  while (str1[i] != '\0' && str2[i] != '\0') {
    if (!irc_charissame(str1[i], str2[i])) {
      return (str1[i] < str2[i]) ? true : false;
    }
    i++;
  }
  return (str1[i] == '\0' && str2[i] == '\0')
             ? false
             : ((str1[i] == '\0') ? true : false);
}

bool channel_key_is_valid(std::string& key) {
  for (size_t i = 0; i < key.size(); ++i) {
    if (key.at(i) == ' ' || key.at(i) == ',' || key.at(i) == 6) return false;
  }
  return true;
}

bool irc_wildcard_cmp(const char* string, const char* mask) {
  while (*string && *mask) {
    if (irc_charissame(*mask, *string) || *mask == '?') {
      mask++;
      string++;
    } else if (*mask == '*') {
      while (*mask == '*') {
        mask++;
      }
      if (*mask == '\0') {
        return true;
      }
      while (*string) {
        if (irc_wildcard_cmp(string, mask)) {
          return true;
        }
        string++;
      }
      return false;
    } else {
      return false;
    }
  }
  return ((*mask == '*' || *mask == '\0') && *string == '\0');
}

void parse_banmask(const std::string& arg, std::string& banmask_nickname,
                   std::string& banmask_username,
                   std::string& banmask_hostname) {
  size_t pos_excl;
  if ((pos_excl = arg.find("!")) != std::string::npos) {
    banmask_nickname = arg.substr(0, pos_excl);
    size_t pos_at;
    if ((pos_at = arg.find("@")) != std::string::npos) {
      banmask_username = arg.substr(pos_excl + 1, pos_at - pos_excl - 1);
      banmask_hostname = arg.substr(pos_at + 1, arg.size() - pos_at - 1);
    } else {
      banmask_username = arg.substr(pos_excl + 1, arg.size() - pos_excl - 1);
      banmask_hostname = banmask_hostname = "*";
    }
  } else {
    banmask_nickname = arg;
    banmask_username = "*";
    banmask_hostname = "*";
  }
}

bool is_valid_userlimit(std::string arg) {
  if (arg.size() > 3) {return false;}
  for (size_t i = 0; i < arg.size(); ++i) {
        if (!isdigit(arg.at(i)))
          return false;
      }
  return true;
}

}  // namespace irc
