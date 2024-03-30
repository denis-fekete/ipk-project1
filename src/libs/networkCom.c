/**
 * @file networkCom.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief Implementation of for working with network connections 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "networkCom.h"

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

int getSocket(enum Protocols protocol)
{   
    int family;
    switch (protocol)
    {
        case prot_UDP: family = SOCK_DGRAM; break; 
        case prot_TCP: family = SOCK_STREAM; break;
        default: errHandling("Unknown protocol passed to function get socket", -1); 
    }

    int newSocket = socket(AF_INET, family, 0);
    if(newSocket < 0) { errHandling("Socket creation failed", 1); /*TODO:*/ }

    return newSocket;
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

struct sockaddr_in findServer(const char* serverHostname, uint16_t serverPort)
{
    //TODO: getaddrinfo(), this is depricated, with freeaddrinfo
    struct hostent* server = gethostbyname(serverHostname);
    if(server == NULL)
    {
        fprintf(stderr, "ERROR: No such host %s\n", serverHostname);
        exit(1); //TODO:
    }

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress)); // clear memory
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    memcpy(&serverAddress.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    
    debugPrint(stdout, "DEBUG: Server socket %s : %d \n", inet_ntoa(serverAddress.sin_addr), ntohs(serverAddress.sin_port));
    
    return serverAddress;
}