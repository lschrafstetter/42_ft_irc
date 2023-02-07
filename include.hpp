#pragma once

#include <iostream>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <map>
#include <vector>
#include <string>

#define BUFSIZE 2048
#define MAX_CLIENTS 10