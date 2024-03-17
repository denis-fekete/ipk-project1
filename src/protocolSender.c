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

        queueLock(sendingQueue);

        Message* msgToBeSend = queueGetMessageNoMutex(sendingQueue);

        // if program is not in open state and message to be send is not auth 
        if(getProgramState(progInt) < fsm_OPEN && 
            msgToBeSend->msgFlags != msg_flag_AUTH)
        {
            // wait for receiver to signal that authentication was confirmed
            queueUnlock(sendingQueue);
            pthread_cond_wait(progInt->threads->rec2SenderCond, progInt->threads->rec2SenderMutex);
            queueLock(sendingQueue);
            // renew message to be send
            msgToBeSend = queueGetMessageNoMutex(sendingQueue);
        }

        // check if message wasn't resend more than udpMaxTries
        if(queueGetSendedCounterNoMutex(sendingQueue) > (progInt->netConfig->udpMaxRetries))
        {
            // check if discarded message wasn't an auth message
            msg_flags flags = queueGetMessageFlagsNoMutex(sendingQueue);
            if(flags == msg_flag_AUTH && getProgramState(progInt) == fsm_AUTH_SENDED)
            {
                safePrintStdout("System: Authetication message request timed out. Please try again.\n");
            }
            #ifdef DEBUG
                debugPrint(stdout, "Deleted message:\n")
                bufferPrint(msgToBeSend->buffer, 2);
            #endif
            queuePopMessageNoMutex(sendingQueue);
            queueUnlock(sendingQueue);
            safePrintStdout("System: Request timed out. Last message was not sent.\n");
            continue;
        }


        queueSetMsgIDNoMutex(sendingQueue, progInt);

        #ifdef DEBUG
            debugPrint(stdout, "DEBUG: Sender (queue len: %li", sendingQueue->len);
            debugPrint(stdout,", tried to send a message:\n");
            bufferPrint(msgToBeSend->buffer, 3); // this is causing seg fault
        #endif

        int bytesTx; // number of sended bytes
        // send buffer to the server 
        bytesTx = sendto(progInt->netConfig->openedSocket, msgToBeSend->buffer->data, 
                        msgToBeSend->buffer->used, flags, progInt->netConfig->serverAddress, 
                        progInt->netConfig->serverAddressSize);


        if(bytesTx < 0)
        {
            errHandling("Sending bytes was not successful", 1); // TODO: change error code
        }

        #ifdef DEBUG
            debugPrint(stdout, "DEBUG: Confirmed message sending\n");
            debugPrintSeparator(stdout);
        #endif

        queueMessageSendedNoMutex(sendingQueue);
        queueUnlock(sendingQueue);
        
        // if authentication is wating to be sended, switch state to sended
        if(getProgramState(progInt) == fsm_AUTH_W82_BE_SENDED)
        {
            if(queueGetMessageFlags(sendingQueue) == msg_flag_AUTH)
            {
                setProgramState(progInt, fsm_AUTH_SENDED);
            }
            else
            {
                errHandling("Sender sended message that isn't AUTH in non-open state", 1); // TODO:
            }
        }

        queueLock(sendingQueue);
        // if DO_NOT_RESEND flags is set, delete msg from queue right away
        if(queueGetMessageFlagsNoMutex(sendingQueue) == msg_flag_DO_NOT_RESEND)
        {
            queuePopMessageNoMutex(sendingQueue);
        }
        queueUnlock(sendingQueue);

        // After message has been sended wait udpTimeout time until 
        // attempting to resend it / again
        TIMEOUT_CALCULATION(progInt->netConfig->udpTimeout);
        pthread_cond_timedwait(progInt->threads->rec2SenderCond, progInt->threads->rec2SenderMutex, &timeToWait);


    // repeat until continueProgram is false and queue is empty
    } while( progInt->threads->continueProgram || !(queueIsEmpty(sendingQueue)) );

    debugPrint(stdout, "DEBUG: Sender ended\n");
    return NULL;
}