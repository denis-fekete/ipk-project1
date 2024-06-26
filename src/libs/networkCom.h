/**
 * @file networkCom.h
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief Declaration of functions and structures for network connections
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

/**
 * @brief Types of protocol that can be used by application 
 */
typedef enum Protocols {prot_ERR=-50, prot_UDP=50, prot_TCP=100} prot_t;

/**
 * @brief Structure holding current network configuration of program
 */
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

/**
 * @brief Sets default values for NetworkConfig
 * 
 * @param config pointer to the netword config
 */
void defaultNetworkConfig(NetworkConfig* config);

#endif