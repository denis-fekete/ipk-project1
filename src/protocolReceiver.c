/**
 * @file protocolReceiver.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "protocolReceiver.h"

extern bool continueProgram;

// use macro for loop overhead because it would be too much tabulators
#define LOOP_WITH_EPOLL_START \
    do { \
        epoll_ctl(epollFd, EPOLL_CTL_ADD, config->openedSocket, &event); \
        int readySockets = epoll_wait(epollFd, events, MAX_EVENTS, -1); \
        if(readySockets){ } /*TODO: delete*/ \
        for(unsigned i = 0; i < MAX_EVENTS; i++) { \
            if (events[i].events & EPOLLIN) {

#define LOOP_WITH_EPOLL_END \
            } \
        } \
    } while (continueProgram);


void* protocolReceiver(void *vargp)
{
    if(vargp){}
    return NULL;
}
/**
 * @brief 
 * 
 * @param vargp 
 * @return void* 
 */
/*
void* protocolReceiver(void *vargp)
{
    // ------------------------------------------------------------------------
    // Set up variables for receiving messages
    // ------------------------------------------------------------------------
    NetworkConfig* config = (NetworkConfig*) vargp;

    uint8_t udpRetries = 0; // number of times that udp retransmission was sent
    BytesBlock commands[4]; // array of commands 

    Buffer serverResponse;
    bufferClear(&serverResponse);
    bufferResize(&serverResponse, 1500); // set max size messages from server

    // Await response
    int flags = 0; // TODO: look if some flags could be used
    uint16_t msgID; // message id incoming
    msg_t msgType; // type of message received

    // ------------------------------------------------------------------------
    // Set up epoll to react to the opened socket
    // ------------------------------------------------------------------------

    #define MAX_EVENTS 3
    struct epoll_event event;
    struct epoll_event events[MAX_EVENTS];
    int epollFd = epoll_create1(0);
    event.events = EPOLLIN; //want to read
    event.data.fd = config->openedSocket;

    // ------------------------------------------------------------------------
    // Set up variables for receiving messages
    // ------------------------------------------------------------------------


    LOOP_WITH_EPOLL_START
    {
        int bytesRx = recvfrom(config->openedSocket, serverResponse.data,
                                serverResponse.allocated, flags, 
                                config->serverAddress, &(config->serverAddressSize));
        
        if(bytesRx <= 0) {errHandling("No bytes received", 1);} // TODO: / DEBUG:
        
        serverResponse.used = bytesRx; //set buffer length (activly used) bytes
        
        disassebleProtocol(&serverResponse, commands, &msgType, &msgID);
        
        #ifdef DEBUG
            printf("Received protocol:\n"); //DEBUG:
            printBuffer(&serverResponse, 0, 1); //DEBUG:
        #endif

        if(msgID == config->comDetails->msgCounter  && msgType == MSG_CONF)
        {
            // increase message counter, only if message was confirmed
            config->comDetails->msgCounter += 1;

            udpRetries = 0; // reset retrasmission counter
        }
        else
        {
            // Check if there was more udp retramissions then max value
            // if yes thow away packet and get new user input  
            if(udpRetries >= config->udpMaxRetries)
            {
                fprintf(stderr, "Packet thrown aways\n"); //TODO: do someting else
                udpRetries = 0;
            }
            else
            {
                // Message was not properly received, try again
                udpRetries += 1;
            }
        }
    }
    LOOP_WITH_EPOLL_END

    free(serverResponse.data);

    return NULL;
}*/