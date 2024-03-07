#include "networkCom.h"

int getSocket(enum Protocols protocol)
{   
    int family;
    switch (protocol)
    {
        case UDP: family = SOCK_DGRAM; break; 
        case TCP: family = SOCK_STREAM; break;
        default: errHandling("Unknown protocol passed to function get socket)", -1); 
    }

    int newSocket = socket(family, AF_INET, 0);
    if(newSocket < 0) { errHandling("Socket creation failed", 1); /*TODO:*/ }

    return newSocket;
}


struct sockaddr_in findServer(const char* serverHostname, uint16_t serverPort)
{
    struct hostent* server = gethostbyname(serverHostname);
    if(server == NULL)
    {
        fprintf(stderr, "ERROR: No such host %s\n", serverHostname);
        exit(1); //TODO:
    }

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    memcpy(&serverAddress.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    
    #ifdef DEBUG
    printf("INFO: Server socket %s : %d \n", inet_ntoa(serverAddress.sin_addr), ntohs(serverAddress.sin_port));
    #endif
    
    return serverAddress;
}