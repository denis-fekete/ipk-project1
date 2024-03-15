/**
 * @file protocolReceiver.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "protocolReceiver.h"
#include "time.h"

extern bool continueProgram;
extern NetworkConfig config;
extern MessageQueue* receivedQueue;
extern MessageQueue* sendingQueue;

extern pthread_cond_t rec2SenderCond;
extern pthread_mutex_t rec2SenderMutex;


// use macro for loop overhead because it would be too much tabulators
#define LOOP_WITH_EPOLL_START \
    do { \
        epoll_ctl(epollFd, EPOLL_CTL_ADD, config->openedSocket, &event);    \
        int readySockets = epoll_wait(epollFd, events, MAX_EVENTS, 1);      \
        if(readySockets){ } /*TODO: delete*/                                \
        for(unsigned i = 0; i < MAX_EVENTS; i++) {                          \
            if (events[i].events & EPOLLIN) {

#define LOOP_WITH_EPOLL_END \
            } \
        } \
    } while (continueProgram);


/**
 * @brief 
 * 
 * @param vargp 
 * @return void* 
 */

void* protocolReceiver(void *vargp)
{
    // ------------------------------------------------------------------------
    // Set up variables for receiving messages
    // ------------------------------------------------------------------------
    NetworkConfig* config = (NetworkConfig*) vargp;

    BytesBlock commands[4]; // array of commands 

    Buffer serverResponse;
    bufferInit(&serverResponse);
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

    struct timeval tv;
    tv.tv_sec = 1;
    // tv.tv_usec = 500000; /*set timeout for recvfrom to 500ms*/
    if(setsockopt(config->openedSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)))
    {
        errHandling("setsockopt() failed\n", 1); //TODO:
    }

    // ------------------------------------------------------------------------
    // Set up variables for receiving messages
    // ------------------------------------------------------------------------

    LOOP_WITH_EPOLL_START
    {
        int bytesRx = recvfrom(config->openedSocket, serverResponse.data,
                                serverResponse.allocated, flags, 
                                config->serverAddress, &(config->serverAddressSize));
        
        if(bytesRx <= 0) {continue;}
        
        serverResponse.used = bytesRx; //set buffer length (activly used) bytes
        
        disassebleProtocol(&serverResponse, commands, &msgType, &msgID);
        
        Buffer* topOfQueue = NULL;
        switch(msgType)
        {
            case MSG_CONF:
                // get pointer to the first queue
                topOfQueue = queueGetMessageNoUnlock(sendingQueue);

                // check if top is not empty
                if(topOfQueue != NULL)
                {
                    u_int16_t queueTopMsgID = convert2BytesToUInt(topOfQueue->data);
                
                    if(msgID == queueTopMsgID)
                    {
                        queuePopMessageNoMutex(sendingQueue);
                        
                        pthread_cond_signal(&rec2SenderCond);
                        printf("DEBUG: Confirmed message: %i\n", queueTopMsgID);
                    }
                }

                queueUnlock(sendingQueue);
                break;
            case MSG_BYE:
                continueProgram = false;
                break;
            default: break;
        }

        #ifdef DEBUG
            printf("Received protocol, type: (%i), contents:\n", msgType); //DEBUG:
            bufferPrint(&serverResponse, 0, 1); //DEBUG:
        #endif
    }
    LOOP_WITH_EPOLL_END

    free(serverResponse.data);
    printf("Receiver ended\n");

    return NULL;
}