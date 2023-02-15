#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <ctime>
#include <map>
#include <vector>
#include <string>
#include <queue>
#include <set>

#define BUFFERSIZE 2048
#define MAX_CLIENTS 10
#define DEBUG 1


namespace irc {

//	helpers.cpp
std::vector<std::string>  split_string(std::string& line, char delim);
bool irc_stringissame(const std::string & str1, const std::string & str2);

template< class T >
struct irc_stringmapcomparator : public std::binary_function<T, T, bool> {
  bool operator()( const T& lhs, const T& rhs ) const {
    return irc_stringissame(lhs, rhs);
  }
};

}