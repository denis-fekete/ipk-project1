/**
 * @file protocolSender.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "protocolSender.h"
#include "sys/time.h"

extern bool continueProgram;
extern NetworkConfig config;
extern MessageQueue* sendingQueue;
extern pthread_cond_t main2SenderCond;
extern pthread_mutex_t main2SenderMutex;
extern pthread_cond_t rec2SenderCond;
extern pthread_mutex_t rec2SenderMutex;

#define TIMEOUT_CALCULATION(millis)                                             \
    gettimeofday(&timeNow, NULL);                                               \
    timeToWait.tv_sec = timeNow.tv_sec + millis / 1000;                         \
    timeToWait.tv_nsec = timeNow.tv_usec * 1000 + 1000 * 1000 * (millis % 1000);\
    timeToWait.tv_sec += timeToWait.tv_nsec / (1000 * 1000 * 1000);             \
    timeToWait.tv_nsec %= (1000 * 1000 * 1000);

void* protocolSender(void* vargp)
{
    if(vargp) {} // Fool compiler

    int flags = 0; //TODO: check if some usefull flags could be done
    if(flags){} // DEBUG:
    
    struct timespec timeToWait; // time variable for timeout calculation
    struct timeval timeNow; // time variable for timeout calculation

    do 
    {
        // if queue is empty wait until it is filled
        if(queueIsEmpty(sendingQueue))
        {
            // use pthread wait for main thread to ping that queue is not 
            // empty or timeout to expire
            pthread_cond_wait(&main2SenderCond, &main2SenderMutex);
            continue;
        }

        // check if message was sended more than maxRetries
        if(queueGetSendedCounter(sendingQueue) > (config.udpMaxRetries))
        {
            queuePopMessage(sendingQueue);//TODO: print some error
            printf("DEBUG: Message thrown away\n");
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

        queueMessageSended(sendingQueue);

        #ifdef DEBUG
            printf("Sended:\n"); // DEBUG:
            bufferPrint(msgToBeSend, 0, 1);
        #endif

        // After message has been sended wait udpTimeout time until 
        // attempting to resend it / again
        TIMEOUT_CALCULATION(config.udpTimeout);
        pthread_cond_timedwait(&rec2SenderCond, &rec2SenderMutex, &timeToWait);


    // repeat until continueProgram is false and queue is empty
    } while( continueProgram || !(queueIsEmpty(sendingQueue)) );

    printf("Sender ended\n");
    return NULL;
}