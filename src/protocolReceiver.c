/**
 * @file protocolReceiver.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "protocolReceiver.h"
#include "sys/time.h"

void assembleConfirmProtocol(Buffer* recvBuffer, Buffer* confirmBuffer, ProgramInterface* progInt)
{
    if(recvBuffer == NULL || confirmBuffer == NULL)
    {
         errHandling("Invalid pointers passed as arguements for assembleConfirmProtocol()\n", 1); // TODO:
    }

    BytesBlock commands[4];
    // set commands values to received message id
    commands[0].start = &(recvBuffer->data[1]);
    commands[0].len = 1;
    commands[1].start = &(recvBuffer->data[2]);
    commands[1].len = 1;

    commands[2].start = NULL; commands[2].len = 0;
    commands[3].start = NULL; commands[3].len = 0;

    bool res = assembleProtocol(cmd_CONF, commands, confirmBuffer, progInt->comDetails, progInt);
    if(!res)
    {
        errHandling("Assembling of confrim protocol failed\n", 1); // TODO:
    }
}


// use macro for loop overhead because it would be too much tabulators
#define LOOP_WITH_EPOLL_START \
    do { \
        epoll_ctl(epollFd, EPOLL_CTL_ADD, progInt->netConfig->openedSocket, &event);    \
        int readySockets = epoll_wait(epollFd, events, MAX_EVENTS, 1);      \
        if(readySockets){ } /*TODO: delete*/                                \
        for(unsigned i = 0; i < MAX_EVENTS; i++) {                          \
            if (events[i].events & EPOLLIN) {

#define LOOP_WITH_EPOLL_END \
            } \
        } \
    } while (progInt->threads->continueProgram);


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
    ProgramInterface* progInt = (ProgramInterface*) vargp;
    MessageQueue* sendingQueue = progInt->threads->sendingQueue;
    fsm_t* fsmState = &(progInt->threads->fsmState);

    BytesBlock commands[4]; // array of commands 

    Buffer serverResponse;
    bufferInit(&serverResponse);
    bufferResize(&serverResponse, 1500); // set max size messages from server
    Buffer sendConfirm;
    bufferInit(&sendConfirm);
    bufferResize(&sendConfirm, 5); // set confirm uses max 3 bytes

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
    event.data.fd = progInt->netConfig->openedSocket;

    struct timeval tv;
    tv.tv_sec = 1;

    if(setsockopt(progInt->netConfig->openedSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)))
    {
        errHandling("setsockopt() failed\n", 1); //TODO:
    }

    // ------------------------------------------------------------------------
    // Set up variables for receiving messages
    // ------------------------------------------------------------------------

    LOOP_WITH_EPOLL_START
    {
        int bytesRx = recvfrom(progInt->netConfig->openedSocket, serverResponse.data,
                                serverResponse.allocated, flags, 
                                progInt->netConfig->serverAddress, 
                                &(progInt->netConfig->serverAddressSize));
        
        if(bytesRx <= 0) {continue;}
        
        serverResponse.used = bytesRx; //set buffer length (activly used) bytes
        
        disassebleProtocol(&serverResponse, commands, &msgType, &msgID);
        
        Buffer* topOfQueue = NULL;

        // if in fsm_AUTH state and incoming msg is not REPLY do nothing
        if(*fsmState == fsm_START && msgType != msg_REPLY && msgType != msg_CONF)
        {
            debugPrint(stdout, "DEBUG: Receiver thrown away a message because"
                "program is in START state and message is not CONFIRM or REPLY");
            continue;
        }

        char replyResult;
        switch(msgType)
        {
            case msg_CONF:
                // get pointer to the first queue
                topOfQueue = queueGetMessageNoUnlock(sendingQueue);

                // check if top is not empty
                if(topOfQueue != NULL)
                {
                    u_int16_t queueTopMsgID = convert2BytesToUInt(topOfQueue->data);
                
                    if(msgID == queueTopMsgID)
                    {
                        queuePopMessageNoMutex(sendingQueue);
                        
                        pthread_cond_signal(progInt->threads->rec2SenderCond);
                        debugPrint(stdout, "DEBUG: Confirmed message: %i\n", queueTopMsgID);
                    }
                }

                queueUnlock(sendingQueue);
                break;
            case msg_REPLY:;
                replyResult = serverResponse.data[3];

                if(*fsmState == fsm_START)
                {
                    if(replyResult == 1)
                    {
                        // assemble confirm protcol
                        assembleConfirmProtocol(&serverResponse, &sendConfirm, progInt);
                        // add it to queue at start
                        queueAddMessagePriority(sendingQueue, &sendConfirm, msg_flag_DO_NOT_RESEND);
                        // ping / signal sender
                        pthread_cond_signal(progInt->threads->senderEmptyQueueCond);
                        // set state to OPEN TODO: what if confirm failed?
                        *fsmState = fsm_OPEN;
                    }
                    else { /*TODO:*/}
                }
                break;
            case msg_BYE:
                progInt->threads->continueProgram = false;
                break;
            default: break;
        }

        debugPrint(stdout, "DEBUG:Received protocol, type: (%i), bytes(%i), contents:\n", msgType, bytesRx);
        bufferPrint(&serverResponse, true); // DEBUG:
    }
    LOOP_WITH_EPOLL_END

    free(serverResponse.data);
    debugPrint(stdout, "DEBUG: Receiver ended\n");

    return NULL;
}