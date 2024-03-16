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

#define TIMEOUT_CALCULATION(millis)                                             \
    gettimeofday(&timeNow, NULL);                                               \
    timeToWait.tv_sec = timeNow.tv_sec + millis / 1000;                         \
    timeToWait.tv_nsec = timeNow.tv_usec * 1000 + 1000 * 1000 * (millis % 1000);\
    timeToWait.tv_sec += timeToWait.tv_nsec / (1000 * 1000 * 1000);             \
    timeToWait.tv_nsec %= (1000 * 1000 * 1000);

void* protocolSender(void* vargp)
{
    ProgramInterface* progInt = (ProgramInterface*) vargp;
    MessageQueue* sendingQueue = progInt->threads->sendingQueue;
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
            pthread_cond_wait(progInt->threads->senderEmptyQueueCond, progInt->threads->senderEmptyQueueMutex);
            continue;
        }
        // check if message was sended more than maxRetries

        if(queueGetSendedCounter(sendingQueue) > (progInt->netConfig->udpMaxRetries))
        {
            queuePopMessage(sendingQueue);//TODO: print some error
            printf("System: Request timed out. Last message was not sent.\n");
            continue;
        }

        Buffer* msgToBeSend = queueGetMessage(sendingQueue);

        int bytesTx; // number of sended bytes

        // send buffer to the server 
        bytesTx = sendto(progInt->netConfig->openedSocket, msgToBeSend->data, 
                        msgToBeSend->used, flags, progInt->netConfig->serverAddress, 
                        progInt->netConfig->serverAddressSize);

        if(bytesTx < 0)
        {
            errHandling("Sending bytes was not successful", 1); // TODO: change error code
        }

        queueMessageSended(sendingQueue);

        #ifdef DEBUG
            printf("Sended:\n"); // DEBUG:
            bufferPrint(msgToBeSend, 0, 1);
        #endif

        // if DO_NOT_RESEND flags is set, delete msg from queue right away
        if(queueGetMessageFlags(sendingQueue) == msg_flag_DO_NOT_RESEND)
        {
            queuePopMessage(sendingQueue);
        }

        // After message has been sended wait udpTimeout time until 
        // attempting to resend it / again
        TIMEOUT_CALCULATION(progInt->netConfig->udpTimeout);
        pthread_cond_timedwait(progInt->threads->rec2SenderCond, progInt->threads->rec2SenderMutex, &timeToWait);


    // repeat until continueProgram is false and queue is empty
    } while( progInt->threads->continueProgram || !(queueIsEmpty(sendingQueue)) );

    printf("Sender ended\n");
    return NULL;
}