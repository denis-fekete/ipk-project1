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
 * @brief Filters messages to by send by currecnt state of program and by
 * MessageType. 
 * 
 * @param progInt Pointer to the ProgramInterface 
 */
void filterMessagesByFSM(ProgramInterface* progInt)
{
    MessageQueue* sendingQueue = progInt->threads->sendingQueue;

    // get msg again in case someone changed first it ... this is primary 
    // for receiver importing priority messages like CONFIRM
    Message* msgToBeSend = queueGetMessage(sendingQueue); 
    debugPrint(stdout, "%p\n", (void*)msgToBeSend); //DEBUG:
    bufferPrint(msgToBeSend->buffer, 9);//DEBUG:

    msg_t msgType = queueGetMessageMsgType(sendingQueue);
    
    msg_flags flags = queueGetMessageFlags(sendingQueue);

    switch (getProgramState(progInt))
    {
    // ------------------ FSM IN BEFORE SUCCESSFULL AUTHENTICATION ------------
    case fsm_START:
    case fsm_AUTH_W82_BE_SENDED: /*authentication is waiting(W8) to(2) be sended*/
    case fsm_AUTH_SENDED: /*authetication was successfully sended*/
    case fsm_W84_REPLY: /*auth has been confirmed, waiting for reply*/
        // if program is not in open state and message to be send is not auth 
        if(msgType != msg_AUTH && flags != msg_flag_NOK_REPLY)
        {
            #ifdef DEBUG
                debugPrint(stdout, "DEBUG: Message that is not auth blocked because of FSM state\n");
                debugPrint(stdout, "type: %i\n", msgType); //DEBUG:
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
        else if(msgType == msg_AUTH && msgToBeSend->confirmed && flags != msg_flag_REJECTED)
        {
            #ifdef DEBUG
                debugPrint(stdout, "DEBUG: AUTH message blocked because of FSM state (state: %i)\n", getProgramState(progInt));
                bufferPrint(msgToBeSend->buffer, 7);
            #endif
            queueUnlock(sendingQueue);
            // message was confirmed, wait for receiver to ping me   
            pthread_cond_wait(progInt->threads->rec2SenderCond, 
                progInt->threads->rec2SenderMutex);
            // call filter again
            filterMessagesByFSM(progInt);
            queueLock(sendingQueue);
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
    case fsm_OPEN:
        switch(msgType)
        {
            case msg_AUTH:
                // TODO: look if okey with assigment
                safePrintStdout("System: You are already autheticated, message will be ignored.");
                queuePopMessage(sendingQueue);
                break;
            case msg_JOIN:
                UDP_VARIANT
                    setProgramState(progInt, fsm_JOIN_ATEMPT);
                TCP_VARIANT
                END_VARIANTS
                break;
            default:
                break;
        }
        break;
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
void filterResentMessages(MessageQueue* sendingQueue, ProgramInterface* progInt)
{
    Message* msgToBeSend = queueGetMessage(sendingQueue);
    while(msgToBeSend != NULL)
    {
        // if message was send more than maximum udp retries
        if( queueGetSendedCounter(sendingQueue) > progInt->netConfig->udpMaxRetries )
        {
            // check if discarded message wasn't an auth message
            msg_flags flags = queueGetMessageFlags(sendingQueue);
            
            // if msg to be deleted is auth
            if(flags == msg_flag_AUTH)
            {
                switch (getProgramState(progInt))
                {
                case fsm_START:
                case fsm_AUTH_W82_BE_SENDED: /*authentication is waiting(W8) to(2) be sended*/
                case fsm_AUTH_SENDED: /*authetication was successfully sended*/
                case fsm_W84_REPLY: /*auth has been confirmed, waiting for reply*/
                case fsm_W84_REPLY_CONF: /*reply received, waiting for confirm to be sended*/
                    safePrintStdout("System: Authetication message request timed out. Please try again.\n");
                    setProgramState(progInt, fsm_START);
                    // signal main to wake up
                    pthread_cond_signal(progInt->threads->mainCond);
                    break;
                default:
                    break;
                }
            } else {
                safePrintStdout("System: Request timed out. Last message was not sent.\n");   
                debugPrint(stdout, "Count: %i\n", queueGetSendedCounter(sendingQueue));
                bufferPrint(msgToBeSend->buffer, 1);
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
        debugPrint(stdout, "DEBUG: Sender at start of the loop\n");
        UDP_VARIANT
            queueLock(sendingQueue);

            filterResentMessages(sendingQueue, progInt);
        TCP_VARIANT
        END_VARIANTS

        // --------------------------------------------------------------------
        // If queue is empty wait for someone to ping sender that msg was added
        // --------------------------------------------------------------------

        // if queue is empty wait until it is filled
        if(queueIsEmpty(sendingQueue))
        {
            // if queue is empty and state is empty queue and bye, end
            if(getProgramState(progInt) == fsm_EMPTY_Q_BYE)
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

        filterMessagesByFSM(progInt);

        Message* msgToBeSend = queueGetMessage(sendingQueue);

        // --------------------------------------------------------------------
        // Send message
        // --------------------------------------------------------------------

        // in udp variant messages must be label with correct message id
        UDP_VARIANT
            // set correct message id right before sending it
            queueSetMessageID(sendingQueue, progInt);
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
            errHandling("Sending bytes was not successful", 1); // TODO: change error code
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
            // in tcp variant always pop message
            queuePopMessage(sendingQueue);
            // if it was message, signal main
            if(msgToBeSend->type == msg_MSG)
            {
                debugPrint(stdout, "DEBUG: MSG Send, waking main\n");
                // ping main to work again
                pthread_cond_signal(progInt->threads->mainCond);
            }
        END_VARIANTS

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