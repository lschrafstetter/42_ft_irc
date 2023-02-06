#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>
#include <vector>
#include <string>
#include "Client.hpp"
#include "Server.hpp"

#define BUFSIZE = 2048;
#define MAX_CLIENTS = 10;