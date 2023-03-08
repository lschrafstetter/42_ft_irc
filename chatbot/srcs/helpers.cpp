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

}  // namespace irc
