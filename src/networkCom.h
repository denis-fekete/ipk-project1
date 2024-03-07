#ifndef NETWORK_COM_H
#define NETWORK_COM_H

#include "arpa/inet.h"
#include "sys/socket.h"
#include "netdb.h"

#include "utils.h"

int getSocket(enum Protocols protocol);


struct sockaddr_in findServer(const char* serverHostname, uint16_t serverPort);

#endif