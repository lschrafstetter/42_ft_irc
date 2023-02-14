#include "Server.hpp"

namespace irc {

std::vector<std::string>  split_std_strings(std::string line, char delim) {
  std::vector<std::string>  ret;
  std::string               buf;
  while (std::getline(ret, buf, delim))
    ret.push_back(buf);
  return ret;
}

}