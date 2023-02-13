#pragma once

#include <iostream>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <map>
#include <vector>
#include <string>
#include <queue>
#include <sstream>

#define BUFFERSIZE 2048
#define MAX_CLIENTS 10
#define DEBUG 1
