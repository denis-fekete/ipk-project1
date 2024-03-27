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

    ProtocolBlocks pBlocks;

    // set commands values to received message id
    pBlocks.cmd_conf_lowMsgID.start = &(recvBuffer->data[1]);
    pBlocks.cmd_conf_lowMsgID.len = 1;

    pBlocks.cmd_conf_highMsgID.start = &(recvBuffer->data[2]);
    pBlocks.cmd_conf_highMsgID.len = 1;

    pBlocks.second.start = NULL; pBlocks.second.len = 0;
    pBlocks.third.start = NULL; pBlocks.third.len = 0;
    pBlocks.type = cmd_CONF;

    bool res = assembleProtocol(&pBlocks, confirmBuffer, progInt);
    if(!res)
    {
        errHandling("Assembling of confrim protocol failed\n", 1); // TODO:
    }
}

void sendConfirm(Buffer* serverResponse, Buffer* confirmBuffer, ProgramInterface* progInt)
{
    assembleConfirmProtocol(serverResponse, confirmBuffer, progInt);

    queueLock(progInt->threads->sendingQueue);
    queueAddMessagePriority(progInt->threads->sendingQueue, confirmBuffer, msg_flag_CONFIRM);
    queueUnlock(progInt->threads->sendingQueue);
}

void receiverFSM(ProgramInterface* progInt, uint16_t msgID, ProtocolBlocks* pBlocks, Buffer* serverResponse,
    MessageQueue* sendingQueue, MessageQueue* confirmedMsgs, Buffer* confirmBuffer)
{
    Message* topOfQueue = NULL;
    char replyResult;
    bool repetitiveMsg = false;

    // ------------------------------------------------------------------------
    // Filter out resend messages
    // ------------------------------------------------------------------------
    char highMsgIDByte;
    char lowMsgIDByte;
    breakU16IntToBytes(&highMsgIDByte, &lowMsgIDByte, msgID);

    queueLock(confirmedMsgs);
    // if message id queue contains received message add it to it and prerform action
    if(! queueContainsMessageId(confirmedMsgs, highMsgIDByte, lowMsgIDByte))
    {
        queueAddMessageOnlyID(confirmedMsgs, serverResponse);
    }
    else // message was already received once, do nothing
    {
        repetitiveMsg = true;
    }
    queueUnlock(confirmedMsgs);

    // ------------------------------------------------------------------------
    // Perform action based on message type
    // ------------------------------------------------------------------------

    switch(uchar2msgType(pBlocks->type))
    {
        case msg_CONF:
            queueLock(sendingQueue);
            // check if top is not empty
            if((topOfQueue = queueGetMessage(sendingQueue)) != NULL)
            {
                uint16_t queueTopMsgID = queueGetMessageID(sendingQueue);
            
                // if first in queue is same as incoming confirm, confirm message
                if(msgID == queueTopMsgID)
                {
                    // confirm message
                    topOfQueue->confirmed = true;
                    // increase global counter message counter
                    progInt->comDetails->msgCounter = msgID + 1;

                    pthread_cond_signal(progInt->threads->rec2SenderCond);

                    debugPrint(stdout, "DEBUG: Msg sended by sender was "
                        "confirmed, id: %i\n", queueTopMsgID);

                    // if auth has been sended, and waiting to be confirmed 
                    if(getProgramState(progInt) == fsm_AUTH_SENDED)
                    {
                        // change state to confirmed, and wait for reply
                        setProgramState(progInt, fsm_W8_4_REPLY);
                    } 
                    // received confirmation of bye
                    else if(getProgramState(progInt) == fsm_BYE_RECV)
                    {
                        // change state to end the program
                        setProgramState(progInt, fsm_END);
                    }
                }
            }

            queueUnlock(sendingQueue);
            break;
        // --------------------------------------------------------------------
        case msg_REPLY:;
            // if it is repetitive message send confirm and do nothing
            if(repetitiveMsg)
            {
                // sendConfirm(serverResponse, confirmBuffer, progInt);
                debugPrint(stdout, "Repetetive reply received\n");
                // return;
            }

            replyResult = serverResponse->data[3];
            debugPrint(stdout, "DEBUG: Reply message resuted in: %i\n", replyResult);

            // if waiting for authetication reply
            if(getProgramState(progInt) == fsm_W8_4_REPLY)
            {
                // replty to auth is positive
                if(replyResult == 1)
                {
                    // set auth message as confirmed
                    queueLock(sendingQueue);
                    queueSetMessageFlags(sendingQueue, msg_flag_CONFIRMED);
                    queueUnlock(sendingQueue);

                    // send confirm message
                    sendConfirm(serverResponse, confirmBuffer, progInt);

                    // set state to OPEN
                    setProgramState(progInt, fsm_OPEN);

                    // safePrintStdout("Connection established\n\n"); // TODO: check if okey with assignment
                    // ping / signal sender
                    pthread_cond_signal(progInt->threads->senderEmptyQueueCond);
                    pthread_cond_signal(progInt->threads->rec2SenderCond);

                }
                else
                {
                    safePrintStdout("Server: %s\n", pBlocks->msg_reply_MsgContents.start);

                    queueLock(sendingQueue);
                    // check if top of queue is AUTH message, reject it
                    if(queueGetMessageFlags(sendingQueue) == msg_flag_AUTH)
                    {
                        // set flag of auth message to rejected
                        queueSetMessageFlags(sendingQueue, msg_flag_REJECTED);
                        // set program state back to start
                        setProgramState(progInt, fsm_START);
                    }
                    queueUnlock(sendingQueue);

                    pthread_cond_signal(progInt->threads->rec2SenderCond);
                }
                // increase counter of messages no matter the response/reply
                progInt->comDetails->msgCounter = msgID + 1;
            }
            break;
        // --------------------------------------------------------------------
        case msg_MSG: // BYE was received
            // if it is repetitive message send confirm and do nothing
            if(repetitiveMsg)
            {
                debugPrint(stdout, "Repetetive message received\n");
                // return;
            }

            sendConfirm(serverResponse, confirmBuffer, progInt);
            
            // ping / signal sender
            pthread_cond_signal(progInt->threads->senderEmptyQueueCond);
            pthread_cond_signal(progInt->threads->rec2SenderCond);

            if(pBlocks->msg_msg_displayname.start != NULL && pBlocks->msg_msg_MsgContents.start != NULL)
            {
                safePrintStdout("%s: %s\n", pBlocks->msg_msg_displayname.start, 
                                pBlocks->msg_msg_MsgContents.start);
            }
            break;
        // --------------------------------------------------------------------
        case msg_BYE: // BYE was received
            // send confirm message
            sendConfirm(serverResponse, confirmBuffer, progInt);
            // set staye to END
            setProgramState(progInt, fsm_END);
            safePrintStdout("System: Press ENTER to exit program\n");
            break;
        case msg_ERR:
            // send confirm message
            sendConfirm(serverResponse, confirmBuffer, progInt);
            // set staye to END
            setProgramState(progInt, fsm_END);
            safePrintStdout("ERR FROM %s: %s\n", "0", "0"); // TODO:
        default: 
            break;
    }
}

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
    ProtocolBlocks pBlocks;

    Buffer serverResponse;
    bufferInit(&serverResponse);
    bufferResize(&serverResponse, 1500); // set max size messages from server
    Buffer confirmBuffer;
    bufferInit(&confirmBuffer);
    bufferResize(&confirmBuffer, 5); // set confirm uses max 3 bytes

    MessageQueue confirmedMsgs;
    queueInit(&confirmedMsgs);

    // Await response
    int flags = 0; // TODO: look if some flags could be used
    uint16_t msgID; // message id incoming

    // ------------------------------------------------------------------------
    // Set up epoll to react to the opened socket
    // ------------------------------------------------------------------------

    

    struct timeval tv;
    tv.tv_usec = 0;
    tv.tv_sec = 1;

    // set timeout for recvfrom in case that program ended and no 
    // more messages will be sent 
    if(setsockopt(progInt->netConfig->openedSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)))
    {
        errHandling("setsockopt() failed\n", 1); //TODO:
    }

    // ------------------------------------------------------------------------
    // Set up variables for receiving messages
    // ------------------------------------------------------------------------
    while(getProgramState(progInt) != fsm_END)
    {
        int bytesRx = recvfrom(progInt->netConfig->openedSocket, serverResponse.data,
                                serverResponse.allocated, flags, 
                                progInt->netConfig->serverAddress, 
                                &(progInt->netConfig->serverAddressSize));
        // receiver timeout expired
        if(bytesRx <= 0) {continue;}

        serverResponse.used = bytesRx; //set buffer length (activly used) bytes
        

        disassebleProtocol(&serverResponse, &pBlocks, &msgID);
        
        #ifdef DEBUG
            debugPrint(stdout, "DEBUG: Received protocol, type: (%i), bytes(%i), contents:\n", pBlocks.type, bytesRx);
            bufferPrint(&serverResponse, 1); // DEBUG:
            debugPrintSeparator(stdout);
        #endif

        // if in fsm_AUTH state and incoming msg is not REPLY do nothing
        if(getProgramState(progInt) == fsm_START && pBlocks.type != msg_REPLY && pBlocks.type != msg_CONF)
        {
            debugPrint(stdout, "DEBUG: Receiver thrown away a message because"
                "program is in START state and message is not CONFIRM or REPLY");
            continue;
        }


        receiverFSM(progInt, msgID, &pBlocks, &serverResponse, 
            progInt->threads->sendingQueue, &confirmedMsgs, &confirmBuffer);
    }

    bufferDestory(&serverResponse);
    bufferDestory(&confirmBuffer);
    queueDestroy(&confirmedMsgs);

    debugPrint(stdout, "DEBUG: Receiver ended\n");

    return NULL;
}



    // TODO: DELETE
    // #define MAX_EVENTS 3
    // struct epoll_event event;
    // struct epoll_event events[MAX_EVENTS];
    // int epollFd = epoll_create1(0);
    // event.events = EPOLLIN; //want to read
    // event.data.fd = progInt->netConfig->openedSocket;

/*
// use macro for loop overhead because it would be too much tabulators
#define LOOP_WITH_EPOLL_START \
    do { \
        epoll_ctl(epollFd, EPOLL_CTL_ADD, progInt->netConfig->openedSocket, &event);    \
        int readySockets = epoll_wait(epollFd, events, MAX_EVENTS, 1);      \
        if(readySockets){ }                                                 \
        for(unsigned i = 0; i < MAX_EVENTS; i++) {                          \
            if (events[i].events & EPOLLIN) {

#define LOOP_WITH_EPOLL_END \
            } \
        } \
    } while (progInt->threads->continueProgram);
*/