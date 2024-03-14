/**
 * @file protocolSender.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "protocolSender.h"

extern bool continueProgram;
extern NetworkConfig config;
extern MessageQueue* sendingQueue;
extern pthread_cond_t pingSenderCond;
extern pthread_mutex_t pingSenderMutex;
extern pthread_cond_t pingMainCond;
extern pthread_mutex_t pingMainMutex;

void* protocolSender(void* vargp)
{
    if(vargp) {} // Fool compiler

    int flags = 0; //TODO: check if some usefull flags could be done
    if(flags){}
    
    do 
    {
        // if queue is empty wait until it is filled
        
        if(queueIsEmpty(sendingQueue))
        {
            printf("waiting\n");
            // use pthread wait to not waste cpu power
            pthread_cond_wait(&pingSenderCond, &pingSenderMutex);
            printf("resuming\n");
            continue;
        }

        Buffer* msgToBeSend = queueGetMessage(sendingQueue);

        int bytesTx; // number of sended bytes

        // send buffer to the server 
        bytesTx = sendto(config.openedSocket, msgToBeSend->data, 
                        msgToBeSend->used, flags, config.serverAddress, 
                        config.serverAddressSize);

        if(bytesTx < 0)
        {
            errHandling("Sending bytes was not successful", 1); // TODO: change error code
        }

        #ifdef DEBUG
            printf("Protocol sended sucessfully\n"); // DEBUG:
            bufferPrint(msgToBeSend, 0, 1);
        #endif

        queuePopMessage(sendingQueue);

    // repeat until continueProgram is false and queue is empty
    } while( continueProgram || !(queueIsEmpty(sendingQueue)) );


    return NULL;
}