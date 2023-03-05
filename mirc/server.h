#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include "statistics.h"

#define BACKLOG 7

void initServer(int argc, char* argv[]);

void serveClient(int clientSock);

#endif
