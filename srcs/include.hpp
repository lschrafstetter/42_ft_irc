#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#define BUFFERSIZE 2048
#define MAX_CLIENTS 10
#define MAX_CHANNELS 10
#define DEBUG 0

namespace irc {

//	helpers.cpp
std::vector<std::string> split_string(std::string& line, char delim);
bool irc_stringissame(const std::string& str1, const std::string& str2);
bool irc_customlesscomparator(const char* str1, const char* str2);
bool irc_wildcard_cmp(const char* string, const char* wildcardstring);
bool channel_key_is_valid(std::string& key);
void parse_banmask(const std::string& arg, std::string& banmask_nickname,
                   std::string& banmask_username,
                   std::string& banmask_hostname);
bool is_valid_userlimit(std::string arg);

template <class T>
struct irc_stringmapcomparator : public std::binary_function<T, T, bool> {
  bool operator()(const T& lhs, const T& rhs) const {
    return irc_customlesscomparator(lhs.c_str(), rhs.c_str());
  }
};

}  // namespace irc