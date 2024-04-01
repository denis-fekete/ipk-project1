/**
 * @file protocolSender.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief Implementation of Protocol Sender that sends messages to server 
 * and changes program state accordingly
 * 
 * This module is meant to work in separated thread. 
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
 * @brief Filters messages to by send by currecnt state of program and by
 * MessageType, also updates current state of program  
 * 
 * @param progInt Pointer to the ProgramInterface 
 */
void logicFSM(ProgramInterface* progInt)
{
    MessageQueue* sendingQueue = progInt->threads->sendingQueue;

    // get msg again in case someone changed first it ... this is primary 
    // for receiver importing priority messages like CONFIRM
    Message* msgToBeSend = queueGetMessage(sendingQueue); 

    msg_t msgType = queueGetMessageMsgType(sendingQueue);
    
    msg_flags flags = queueGetMessageFlags(sendingQueue);

    switch (getProgramState(progInt))
    {
    // ------------------ FSM IN BEFORE SUCCESSFULL AUTHENTICATION ------------
    case fsm_START:
    case fsm_AUTH_W82_BE_SENDED: /*authentication is waiting(W8) to(2) be sended*/
    case fsm_AUTH_SENDED: /*authetication was successfully sended*/
    case fsm_W84_REPLY: /*auth has been confirmed, waiting for reply*/
        // if authentication is wating to be sended, switch state to sended
        if( flags == msg_flag_AUTH &&
            getProgramState(progInt) <= fsm_AUTH_W82_BE_SENDED )
        {
            // this could have been through fallthrough however compiler keeps complaining
            setProgramState(progInt, fsm_AUTH_SENDED);
        }

        // if program is not in open state and message to be send is not auth 
        if(msgType != msg_AUTH && flags != msg_flag_NOK_REPLY)
        {
            #ifdef DEBUG
                debugPrint(stdout, "DEBUG: Message that is not auth blocked because of FSM state\n");
                bufferPrint(msgToBeSend->buffer, 9);
            #endif
            queueUnlock(sendingQueue);
            // wait for receiver to signal that authentication was confirmed
            pthread_cond_wait(progInt->threads->rec2SenderCond, 
                progInt->threads->rec2SenderMutex);

            queueLock(sendingQueue);
        }
        // if message is AUTH and was already confirmed, ...
        // wait to prevent repetitive auth sending, and if message wasnt rejected
        else if(msgType == msg_AUTH && msgToBeSend->confirmed && msgToBeSend->sendCount > 0)
        {
            #ifdef DEBUG
                debugPrint(stdout, "DEBUG: AUTH message blocked because of FSM state (state: %i)\n", getProgramState(progInt));
                bufferPrint(msgToBeSend->buffer, 7);
            #endif
            queueUnlock(sendingQueue);
            // message was confirmed, wait for receiver to ping me   
            pthread_cond_wait(progInt->threads->rec2SenderCond, 
                progInt->threads->rec2SenderMutex);
            queueLock(sendingQueue);
            // call filter again
            logicFSM(progInt);
        }
        break;
    // ------------------------------------------------------------------------
    case fsm_W84_REPLY_CONF: /*reply received, waiting for confirm to be sended*/
        // if message to be send is confirm
        if(msgType == msg_CONF)
        {
            // change state to open
            setProgramState(progInt, fsm_OPEN);
            // singal main to start processing another input
            pthread_cond_signal(progInt->threads->mainCond);
        }
        break;
    // ------------------------------------------------------------------------
    case fsm_OPEN:
        switch(msgType)
        {
            case msg_AUTH:
                safePrintStderr("ERR: You are already autheticated, this message will be ignored.");
                queuePopMessage(sendingQueue);
                break;
            case msg_JOIN:
                setProgramState(progInt, fsm_JOIN_ATEMPT);
                break;
            default:
                break;
        }
        break;
    // ------------------------------------------------------------------------
    case fsm_ERR_W84_CONF:
    case fsm_ERR:
            // sended message flags is bye
            if(flags == msg_flag_BYE)
            {
                UDP_VARIANT
                    // in UDP you must wait for confirmation of BYE
                    setProgramState(progInt, fsm_END_W84_CONF);
                TCP_VARIANT
                    // end set state to end
                    setProgramState(progInt, fsm_END);
                    pthread_cond_signal(progInt->threads->mainCond);
                END_VARIANTS
            }
            else if(flags == msg_flag_ERR)
            {
                UDP_VARIANT
                    setProgramState(progInt, fsm_ERR_W84_CONF);
                END_VARIANTS
            }
            else if(flags == msg_flag_CONFIRM) {} // if sended is confirm
            else
            {
                errHandling("ERR: Sender send message that is not "
                "BYE in ERROR state\n", err_INTERNAL_BAD_ARG);
            }
    default:
        break;
    }
}

/**
 * @brief Filters out messages from provided queue that were sent
 *  more than specified times in NetworkConfiguration or that are
 * already confirmed
 * 
 * @param sendingQueue Pointer to queue from which should messages be filtered 
 * @param progInt Pointer to ProgramInterface
 */
bool filterResentMessages(MessageQueue* sendingQueue, ProgramInterface* progInt)
{
    bool resetLoop = false;
    Message* msgToBeSend = queueGetMessage(sendingQueue);
    while(msgToBeSend != NULL)
    {
        // if message was send more than maximum udp retries
        if( queueGetSendedCounter(sendingQueue) > progInt->netConfig->udpMaxRetries )
        {
            // if timedout message is ERR pop it and try to send BYE atleast
            if(getProgramState(progInt) == fsm_ERR_W84_CONF) { }
            
            // if timedout message is BYE set program to END and exit, 
            // confirmation wont come ...
            else if(getProgramState(progInt) == fsm_END_W84_CONF) 
            { 
                setProgramState(progInt, fsm_END);
                // signal main to end
                pthread_cond_signal(progInt->threads->mainCond);
                resetLoop = true; // reset loop to not get stuck
            }
            else
            {
                // print error message
                safePrintStderr("ERR: Request timed out.\n");
                // clear message queue
                queuePopAllMessages(sendingQueue);
                // set program into error state
                setProgramState(progInt, fsm_ERR);

                queueUnlock(sendingQueue);
                // add error message to queue
                sendError(&(progInt->cleanUp->protocolToSendedBySender), 
                    progInt, "Request timed out");
                // add bye message to queue
                sendBye(progInt);

                // signal main to end
                pthread_cond_signal(progInt->threads->mainCond);

                queueLock(sendingQueue);
                break;
            }
        }
        else if(msgToBeSend->msgFlags == msg_flag_REJECTED)
        {
            #ifdef DEBUG
                debugPrint(stdout, "Message got rejected, deleting it\n");
                bufferPrint(msgToBeSend->buffer, 1);
            #endif
        }
        // do not throw away auth messages even if they are confirmed
        else if(msgToBeSend->msgFlags == msg_flag_AUTH)
        {
            break;
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

    return resetLoop;
}

/**
 * @brief Initializes protocol sending functionality 
 * 
 * @param vargp arguments
 * @return void* 
 */
void* protocolSender(void* vargp)
{
    ProgramInterface* progInt = (ProgramInterface*) vargp;
    MessageQueue* sendingQueue = progInt->threads->sendingQueue;
    int flags = 0;
    
    struct timespec timeToWait; // time variable for timeout calculation
    struct timeval timeNow; // time variable for timeout calculation

    while( getProgramState(progInt) != fsm_END) 
    {
        // --------------------------------------------------------------------
        // Filter out confirmed messages or messages with too many resends
        // --------------------------------------------------------------------
        UDP_VARIANT
            queueLock(sendingQueue);

            // if BYE was not confirmed and popped, it changed program state 
            // to END, to prevent being stuck resetLopp is set
            bool resetLoop = filterResentMessages(sendingQueue, progInt);
            if(resetLoop) { continue; }
        TCP_VARIANT
        END_VARIANTS

        // --------------------------------------------------------------------
        // If queue is empty wait for someone to ping sender that msg was added
        // --------------------------------------------------------------------

        // if queue is empty wait until it is filled
        if(queueIsEmpty(sendingQueue))
        {
            debugPrint(stdout, "DEBUG: Sender: queue is empty\n");
            // if queue is empty and state is empty queue and bye, end
            if(getProgramState(progInt) == fsm_EMPTY_Q_BYE || getProgramState(progInt) == fsm_SIGINT_BYE)
            {
                // bye was sended, end program
                setProgramState(progInt, fsm_END);
                queueUnlock(sendingQueue);
                // signal main to end as well
                pthread_cond_signal(progInt->threads->mainCond);
                continue; // jump to while condition and end
            }
            else // else wait for someone to ping me
            {
                // use pthread wait for main thread to ping that queue is not 
                // empty or timeout to expire
                debugPrint(stdout, "DEBUG: Sender waiting (queue empty)\n");
                queueUnlock(sendingQueue);
                pthread_cond_wait(progInt->threads->senderEmptyQueueCond, 
                    progInt->threads->senderEmptyQueueMutex);
                continue;
            }
        }
        
        // make room for someone who is waiting
        queueUnlock(sendingQueue);

        // --------------------------------------------------------------------
        // Filter messages by current state of program
        // --------------------------------------------------------------------

        queueLock(sendingQueue);

        logicFSM(progInt);

        Message* msgToBeSend = queueGetMessage(sendingQueue);

        // --------------------------------------------------------------------
        // Send message
        // --------------------------------------------------------------------

        // in udp variant messages must be label with correct message id
        UDP_VARIANT
            // set correct message id right before sending it
            queueSetMessageID(sendingQueue, progInt);
        TCP_VARIANT
            progInt->comDetails->msgCounter += 1;
        END_VARIANTS

        #ifdef DEBUG
            debugPrint(stdout, "DEBUG: Sender (queue len: %li): ", sendingQueue->len);
            bufferPrint(msgToBeSend->buffer, 3);
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
            errHandling("Sending bytes was not successful", err_COMMUNICATION);
        }

        UDP_VARIANT
            // pop messages that should not be resend again
            switch (sendedMessageFlags)
            {
            case msg_flag_DO_NOT_RESEND: /*general do not resend*/
            case msg_flag_CONFIRM: /*confirm*/
            case msg_flag_NOK_REPLY: /*confirm to bad reply*/
                queuePopMessage(sendingQueue);
                break;
            default: /*other: increase sended message counter*/
                queueMessageSended(sendingQueue);
                break;
            }
        TCP_VARIANT
            // if it was message, signal main
            if(msgToBeSend->type == msg_MSG)
            {
                // ping main to work again
                pthread_cond_signal(progInt->threads->mainCond);
            }
            // in tcp variant always pop message
            queuePopMessage(sendingQueue);
        END_VARIANTS

        queueUnlock(sendingQueue);

        // After message has been sended wait udpTimeout time until 
        // attempting to resend it / again
        // only receiver can signal sender from this state
        TIMEOUT_CALCULATION(progInt->netConfig->udpTimeout);
        pthread_cond_timedwait(progInt->threads->rec2SenderCond, progInt->threads->rec2SenderMutex, &timeToWait);

    }

    debugPrint(stdout, "DEBUG: Sender ended\n");
    return NULL;
}