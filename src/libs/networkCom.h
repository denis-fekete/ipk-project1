/**
 * @file networkCom.h
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef NETWORK_COM_H
#define NETWORK_COM_H

#include "arpa/inet.h"
#include "sys/socket.h"
#include "netdb.h"

#include "utils.h"

typedef struct NetworkConfig {
    prot_t protocol;
    uint16_t portNumber;
    uint16_t udpTimeout;
    uint8_t udpMaxRetries;
    int openedSocket;
    struct sockaddr* serverAddress;
    unsigned serverAddressSize;
} NetworkConfig;

#define PORT_NUMBER 4567

#define DEFAULT_NETWORK_CONFIG(config)  \
    config->protocol = prot_ERR;         \
    config->portNumber = PORT_NUMBER;    \
    config->udpTimeout = 250;            \
    config->udpMaxRetries = 3;           \
    config->openedSocket = -1;           \
    config->serverAddress = NULL;        \
    config->serverAddressSize = 0;
/**
 * @brief Get the Socket id
 * 
 * @param protocol Protocol that will be used to open socket (UDP or TCP) 
 * @return int Socket id
 */
int getSocket(prot_t protocol);

/**
 * @brief Finds server and returns it socket adress
 * 
 * @param serverHostname String containing server hostname
 * @param serverPort Port of server
 * @return struct sockaddr_in Socket adress that was created
 */
struct sockaddr_in findServer(const char* serverHostname, uint16_t serverPort);

#endif