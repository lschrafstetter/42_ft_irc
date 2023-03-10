#pragma once

// C
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

// Network
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

// Epoll
#include <sys/epoll.h>

// C++
#include <cstring>
#include <iostream>
#include <sstream>
#include <ctime>
#include <queue>
#include <string>
#include <vector>

#define BUFFERSIZE 2048
#define DEBUG 0

namespace irc {

std::vector<std::string> split_string(std::string& line, char delim);
bool irc_stringissame(const std::string& str1, const std::string& str2);

}  // namespace irc