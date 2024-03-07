#ifndef NETWORK_COM_H
#define NETWORK_COM_H

#include "arpa/inet.h"
#include "sys/socket.h"
#include "netdb.h"

#include "utils.h"

/**
 * @brief Get the Socket id
 * 
 * @param protocol Protocol that will be used to open socket (UDP or TCP) 
 * @return int Socket id
 */
int getSocket(enum Protocols protocol);

/**
 * @brief Finds server and returns it socket adress
 * 
 * @param serverHostname String containing server hostname
 * @param serverPort Port of server
 * @return struct sockaddr_in Socket adress that was created
 */
struct sockaddr_in findServer(const char* serverHostname, uint16_t serverPort);

#endif