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

/**
 * @brief Filters out messages from provided queue that were sent
 *  more than specified times in NetworkConfiguration or that are
 * already confirmed
 * 
 * @param sendingQueue Pointer to queue from which should messages be filtered 
 * @param progInt Pointer to ProgramInterface
 */
void filterOutMessages(MessageQueue* sendingQueue, ProgramInterface* progInt)
{
    Message* msgToBeSend = queueGetMessage(sendingQueue);
    while(msgToBeSend != NULL)
    {
        // if message was send more than maximum udp retries
        if( queueGetSendedCounter(sendingQueue) > progInt->netConfig->udpMaxRetries )
        {
            // check if discarded message wasn't an auth message
            msg_flags flags = queueGetMessageFlags(sendingQueue);
            
            if(flags == msg_flag_AUTH && getProgramState(progInt) == fsm_AUTH_SENDED) {
                safePrintStdout("System: Authetication message request timed out. Please try again.\n");
            } else {
                safePrintStdout("System: Request timed out. Last message was not sent.\n");   
            }
        }
        else if(msgToBeSend->msgFlags == msg_flag_REJECTED)
        {
            #ifdef DEBUG
                debugPrint(stdout, "Message got rejected, deleting it\n");
                bufferPrint(msgToBeSend->buffer, 1);
            #endif
        }
        // if message wasnt already confirmed
        else if (msgToBeSend->confirmed == false)
        {
            // try send it again
            break;
        }

        // delete confirmed message
        queuePopMessage(sendingQueue);

        // get new one message
        msgToBeSend = queueGetMessage(sendingQueue);
    }
}

void* protocolSender(void* vargp)
{
    ProgramInterface* progInt = (ProgramInterface*) vargp;
    MessageQueue* sendingQueue = progInt->threads->sendingQueue;
    int flags = 0; //TODO: check if some usefull flags could be done
    if(flags){} // DEBUG:
    
    struct timespec timeToWait; // time variable for timeout calculation
    struct timeval timeNow; // time variable for timeout calculation

    while( getProgramState(progInt) != fsm_END ) 
    {
        // --------------------------------------------------------------------
        // Filter out confirmed messages or messages with too many resends
        // --------------------------------------------------------------------
     
        queueLock(sendingQueue);

        filterOutMessages(sendingQueue, progInt);

        // make room for someone who is waiting
        queueUnlock(sendingQueue);

        // --------------------------------------------------------------------
        // If queue is empty wait for someone to ping sender that msg was added
        // --------------------------------------------------------------------

        queueLock(sendingQueue);
        // if queue is empty wait until it is filled
        if(queueIsEmpty(sendingQueue))
        {
            // if queue is empty and state is empty queue and bye, end
            if(getProgramState(progInt) == fsm_EMPTY_Q_BYE)
            {
                // bye was sended, end program
                setProgramState(progInt, fsm_END);
                queueUnlock(sendingQueue);
                continue;
            }
            else // else wait for someone to ping me
            {
                // ping main to stop waiting if waiting
                pthread_cond_signal(progInt->threads->mainCond);

                queueUnlock(sendingQueue);
                // use pthread wait for main thread to ping that queue is not 
                // empty or timeout to expire
                pthread_cond_wait(progInt->threads->senderEmptyQueueCond, 
                    progInt->threads->senderEmptyQueueMutex);
                continue;
            }
        }
        
        queueUnlock(sendingQueue);
        // make room for someone who is waiting

        // --------------------------------------------------------------------
        // If in state before OPEN, do not send messages
        // --------------------------------------------------------------------

        queueLock(sendingQueue);

        // get msg again in case someone changed first it ... this is primary 
        // for receiver importing priority messages like CONFIRM
        Message* msgToBeSend = queueGetMessage(sendingQueue); 
        
        // if program is not in open state and message to be send is not auth 
        switch (getProgramState(progInt))
        {
        case fsm_START:
        case fsm_AUTH_W82_BE_SENDED: /*authentication is waiting(W8) to(2) be sended*/
        case fsm_AUTH_SENDED: /*authetication was successfully sended*/
        case fsm_W8_4_REPLY: /*auth has been confirmed, waiting for reply*/
            if(queueGetMessageFlags(sendingQueue) != msg_flag_AUTH)
            {
                queueUnlock(sendingQueue);
                // wait for receiver to signal that authentication was confirmed
                pthread_cond_wait(progInt->threads->rec2SenderCond, 
                    progInt->threads->rec2SenderMutex);

                queueLock(sendingQueue);
                msgToBeSend = queueGetMessage(sendingQueue); 
            }
            break;
        default:
            break;
        }
        // if(getProgramState(progInt) < fsm_OPEN)
        // {
        //     f(queueGetMessageFlags(sendingQueue) != msg_flag_AUTH)
        //     {
        //         queueUnlock(sendingQueue);
        //         // wait for receiver to signal that authentication was confirmed
        //         pthread_cond_wait(progInt->threads->rec2SenderCond, 
        //             progInt->threads->rec2SenderMutex);

        //         queueLock(sendingQueue);
        //         msgToBeSend = queueGetMessage(sendingQueue); 
        //     }
        // }

        // --------------------------------------------------------------------
        // Send message
        // --------------------------------------------------------------------

        // set correct message id right before sending it
        queueSetMsgID(sendingQueue, progInt);

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

        // store sended messeges's flags
        msg_flags sendedMessageFlags = queueGetMessageFlags(sendingQueue);

        if(bytesTx < 0)
        {
            errHandling("Sending bytes was not successful", 1); // TODO: change error code
        }

        #ifdef DEBUG
            debugPrint(stdout, "DEBUG: Confirmed message sending\n");
            debugPrintSeparator(stdout);
        #endif


        // if DO_NOT_RESEND flags is set, delete msg from queue right away
        if(sendedMessageFlags == msg_flag_DO_NOT_RESEND)
        {
            queuePopMessage(sendingQueue);
        }
        else
        {
            queueMessageSended(sendingQueue);
        }

        queueUnlock(sendingQueue);

        
        // --------------------------------------------------------------------
        // Update FSM
        // --------------------------------------------------------------------
        
        switch (getProgramState(progInt))
        {
        case fsm_AUTH_W82_BE_SENDED:
            // if authentication is wating to be sended, switch state to sended
            if(sendedMessageFlags == msg_flag_AUTH)
            {
                setProgramState(progInt, fsm_AUTH_SENDED);
            }
            else
            {
                errHandling("Sender sended message that isn't AUTH in non-open state", 1); // TODO:
            }
            break;
        
        default:
            break;
        }

        // After message has been sended wait udpTimeout time until 
        // attempting to resend it / again
        // only receiver can signal sender from this state
        TIMEOUT_CALCULATION(progInt->netConfig->udpTimeout);
        pthread_cond_timedwait(progInt->threads->rec2SenderCond, progInt->threads->rec2SenderMutex, &timeToWait);

    // repeat until continueProgram is false and queue is empty
    }

    debugPrint(stdout, "DEBUG: Sender ended\n");
    return NULL;
}