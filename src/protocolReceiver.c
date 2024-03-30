/**
 * @file protocolReceiver.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 */

#include "protocolReceiver.h"
#include "sys/time.h"

/**
 * @brief Create confirm protocol
 * 
 * @param serverResponse Buffer from which will referenceID be taken
 * @param receiverSendMsgs Buffer to which should message be stored
 * @param progInt Pointer to Program Interface
 * @param flags Flags to be added to the message
 */
void sendConfirm(Buffer* serverResponse, Buffer* receiverSendMsgs, ProgramInterface* progInt, msg_flags flags)
{
    if(serverResponse == NULL || receiverSendMsgs == NULL)
    {
         errHandling("Invalid pointers passed as arguements for assembleConfirmProtocol()\n", 1); // TODO:
    }

    // resize receiver buffer to size 4, CONFIRM(1 Byte)|MessageID(2 Bytes)
    bufferResize(receiverSendMsgs, 4);

    ProtocolBlocks pBlocks;

    // set commands values to received message id
    pBlocks.cmd_conf_lowMsgID.start = &(serverResponse->data[1]);
    pBlocks.cmd_conf_lowMsgID.len = 1;

    pBlocks.cmd_conf_highMsgID.start = &(serverResponse->data[2]);
    pBlocks.cmd_conf_highMsgID.len = 1;

    pBlocks.second.start = NULL; pBlocks.second.len = 0;
    pBlocks.third.start = NULL; pBlocks.third.len = 0;
    pBlocks.type = cmd_CONF;

    bool res = assembleProtocolUDP(&pBlocks, receiverSendMsgs, progInt);
    if(!res)
    {
        errHandling("Assembling of confrim protocol failed\n", 1); // TODO:
    }

    queueAddMessagePriority(progInt->threads->sendingQueue, receiverSendMsgs, flags);
}

/**
 * @brief 
 * 
 * @param serverResponse 
 * @param receiverSendMsgs 
 * @param progInt 
 */
void sendError(Buffer* serverResponse, Buffer* receiverSendMsgs, ProgramInterface* progInt, const char* message)
{
    if(serverResponse == NULL || receiverSendMsgs == NULL)
    {
         errHandling("Invalid pointers passed as arguements for assembleConfirmProtocol()\n", 1); // TODO:
    }

    size_t messageLen = strlen(message); // -1 == do not count '\0'
    // +5 is overhead for msgType, msgID, 0 zero bytes
    size_t newSize = messageLen + progInt->comDetails->displayName.used + 5; 
    bufferResize(receiverSendMsgs, newSize); // resize buffer

    ProtocolBlocks pBlocks;
    pBlocks.type = cmd_ERR;
    pBlocks.cmd_err_MsgContents.start = (char*) message;
    pBlocks.cmd_err_MsgContents.len = messageLen;

    bool res; 
    UDP_VARIANT
        res = assembleProtocolUDP(&pBlocks, receiverSendMsgs, progInt);
    TCP_VARIANT
        res = assembleProtocolTCP(&pBlocks, receiverSendMsgs, progInt);        
    END_VARIANTS

    if(!res)
    {
        errHandling("Assembling of error protocol failed\n", 1); // TODO:
    }
    queueAddMessagePriority(progInt->threads->sendingQueue, receiverSendMsgs, msg_flag_CONFIRM);
}

/**
 * @brief Function to handle received UDP confirms 
 * 
 * @param progInt Global Program Interface
 * @param msgID ID received message ID
 * @param sendingQueue Pointer to the MessageQueue that will be sended by sender
 */
void handleConfirmUDP(ProgramInterface* progInt, MessageQueue* sendingQueue, u_int16_t msgID)
{
    Message* topOfQueue; 

    queueLock(sendingQueue);
    // check if top is not empty
    if((topOfQueue = queueGetMessage(sendingQueue)) == NULL)
    { queueUnlock(sendingQueue); return; }

    uint16_t queueTopMsgID = queueGetMessageID(sendingQueue);
    
    // if first in queue is same as incoming confirm, confirm message
    if(msgID != queueTopMsgID)
    { 
        queueUnlock(sendingQueue);
        debugPrint(stdout, "DEBUG: Not correct message ID\n");
        return;
    }

    // confirm message
    topOfQueue->confirmed = true;
    // increase global counter message counter
    progInt->comDetails->msgCounter = msgID + 1;

    pthread_cond_signal(progInt->threads->rec2SenderCond);

    debugPrint(stdout, "DEBUG: Msg sended by sender was "
        "confirmed, id: %i\n", queueTopMsgID);

    switch (getProgramState(progInt))
    {
    // if auth has been sended, and it was confirmed (now), change
    // state to be waiting for REPLY to be received as well 
    case fsm_AUTH_SENDED:
        // change state to confirmed, and wait for reply
        setProgramState(progInt, fsm_W84_REPLY);
        break;
    // received confirmation of bye
    case fsm_BYE_RECV:
        // change state to end the program
        setProgramState(progInt, fsm_END);
        break;
    case fsm_OPEN: // signal main that next message can be processed
        // client send message and is waiting for confirm
        pthread_cond_signal(progInt->threads->mainCond);
        break;
    default:
        break;
    }
        
    queueUnlock(sendingQueue);
    return;
}

/**
 * @brief Function to handle received UDP replies 
 * 
 * @param progInt Global Program Interface
 * @param msgID ID received message ID
 * @param pBlocks ProtocolBlocks that holds dissasembled data from message
 * @param sendingQueue Pointer to the MessageQueue that will be sended by sender
 * @param serverResponse Buffer that holds server response
 * @param receiverSendMsgs Buffer for sending confirm messages
 */
void handleReplyUDP( ProgramInterface* progInt, uint16_t msgID, ProtocolBlocks* pBlocks, MessageQueue* sendingQueue,
    Buffer* serverResponse, Buffer* receiverSendMsgs)
{
    // if waiting for authetication reply
    if(getProgramState(progInt) == fsm_W84_REPLY || getProgramState(progInt) == fsm_JOIN_ATEMPT)
    {
        // replty to auth is positive
        if(*(pBlocks->msg_reply_result.start) == 1)
        {
            // if auth
            if(getProgramState(progInt) == fsm_W84_REPLY)
            {
                // set auth message as confirmed
                queueLock(sendingQueue);
                queueSetMessageFlags(sendingQueue, msg_flag_CONFIRMED);
                queueUnlock(sendingQueue);
            }

            // set state to OPEN
            setProgramState(progInt, fsm_W84_REPLY_CONF);

            queueLock(sendingQueue);
            // send confirm message
            sendConfirm(serverResponse, receiverSendMsgs, progInt, msg_flag_CONFIRM);
            queueUnlock(sendingQueue);

            // ping / signal sender
            pthread_cond_signal(progInt->threads->senderEmptyQueueCond);
            pthread_cond_signal(progInt->threads->rec2SenderCond);

            safePrintStderr("Success: %s\n", pBlocks->msg_reply_MsgContents.start);
        }
        else
        {
            queueLock(sendingQueue);
            // check if top of queue is AUTH message, reject it
            if(queueGetMessageFlags(sendingQueue) == msg_flag_AUTH)
            {
                // set flag of auth message to rejected
                queueSetMessageFlags(sendingQueue, msg_flag_REJECTED);
                // set program state back to start
                setProgramState(progInt, fsm_START);
            }
            // TODO: check if this is
            if(getProgramState(progInt) == fsm_JOIN_ATEMPT)
            {
                setProgramState(progInt, fsm_OPEN);
            }
            
            // send confirm message
            sendConfirm(serverResponse, receiverSendMsgs, progInt, msg_flag_NOK_REPLY);
            queueUnlock(sendingQueue);

            // signal sender that new message has been added

            safePrintStderr("Failure: %s\n", pBlocks->msg_reply_MsgContents.start);

            // signal main that it can start working again
            pthread_cond_signal(progInt->threads->mainCond);
        }
        // increase counter of messages no matter the response/reply
        progInt->comDetails->msgCounter = msgID + 1;
    }
}

/**
 * @brief Fininite State Machine logic
 * 
 * @param progInt Global Program Interface
 * @param msgID ID received message ID
 * @param pBlocks ProtocolBlocks that holds dissasembled data from message
 * @param sendingQueue Pointer to the MessageQueue that will be sended by sender
 * @param serverResponse Buffer that holds server response
 * @param confirmedMsgs Pointer to MessageQueue that holds confirmed messsages
 * @param receiverSendMsgs Buffer for sending confirm messages
 * @param noerr Boolean value whenever dissasembling ended sucessful or not
 */
void receiverFSM(ProgramInterface* progInt, uint16_t msgID, ProtocolBlocks* pBlocks, MessageQueue* sendingQueue,
    Buffer* serverResponse, MessageQueue* confirmedMsgs, Buffer* receiverSendMsgs, bool noerr)
{
    bool repetitiveMsg = false;

    // ------------------------------------------------------------------------
    // Filter out resend messages
    // ------------------------------------------------------------------------
    UDP_VARIANT
        char highMsgIDByte;
        char lowMsgIDByte;
        breakU16IntToBytes(&highMsgIDByte, &lowMsgIDByte, msgID);

        queueLock(confirmedMsgs);
        // if message id queue contains received message add it to it and prerform action
        if( pBlocks->type != msg_CONF && !queueContainsMessageId(confirmedMsgs, highMsgIDByte, lowMsgIDByte))
        {
            queueAddMessageOnlyID(confirmedMsgs, serverResponse);
        }
        else // message was already received once, do nothing
        {
            repetitiveMsg = true;
        }
        queueUnlock(confirmedMsgs);
    TCP_VARIANT
        if(!noerr)
        {
            sendError(serverResponse, receiverSendMsgs, progInt, "Received bad message"); //TODO:
        }
    END_VARIANTS

    // ------------------------------------------------------------------------
    // Perform action based on message type
    // ------------------------------------------------------------------------

    switch(uchar2msgType(pBlocks->type))
    {
        case msg_CONF:
        UDP_VARIANT
            handleConfirmUDP(progInt, sendingQueue, msgID);
        TCP_VARIANT
            errHandling("Received CONFIRM message in TCP mode", 1); //TODO: err code change
        END_VARIANTS
            break;
        // --------------------------------------------------------------------
        case msg_REPLY:;

            UDP_VARIANT
                // if it is repetitive message send confirm and do nothing
                if(repetitiveMsg)
                {
                    queueLock(sendingQueue);
                    sendConfirm(serverResponse, receiverSendMsgs, progInt, msg_flag_CONFIRM);
                    queueUnlock(sendingQueue);
                    debugPrint(stdout, "Repetetive reply received\n");
                    return;
                }
                handleReplyUDP(progInt, msgID, pBlocks, sendingQueue, serverResponse, receiverSendMsgs);
            TCP_VARIANT
                switch (getProgramState(progInt))
                {
                // reply received for AUTH OR JOIN
                case fsm_AUTH_SENDED:
                case fsm_JOIN_ATEMPT:
                    // if result is ok, print success to STDERR and change state to OPEN
                    if(pBlocks->msg_reply_result_bool == true)
                    {
                        safePrintStderr("Success: %s\n", pBlocks->msg_reply_MsgContents.start);
                        setProgramState(progInt, fsm_OPEN);
                    }
                    else
                    {
                        // if result is not ok, print failure to STDERR and change state
                        safePrintStderr("Failure: %s\n", pBlocks->msg_reply_MsgContents.start);
                        if(getProgramState(progInt) == fsm_JOIN_ATEMPT)
                        {
                            // TODO: check if okey with assignment
                            setProgramState(progInt, fsm_OPEN);
                        }
                        else
                        {
                            // set state to start because auth wasn't sucessfull
                            setProgramState(progInt, fsm_START);
                        }
                    }
                    break;
                default:
                    break;
                }

                // signal main that it can start working again
                pthread_cond_signal(progInt->threads->mainCond);
            END_VARIANTS
           
            break;
        // --------------------------------------------------------------------
        case msg_MSG: // BYE was received
            UDP_VARIANT
                queueLock(sendingQueue);
                sendConfirm(serverResponse, receiverSendMsgs, progInt, msg_flag_CONFIRM);
                queueUnlock(sendingQueue);
                
                // if it is repetitive message send confirm and do nothing
                if(repetitiveMsg)
                {
                    debugPrint(stdout, "Repetetive message received\n");
                    return;
                }
            END_VARIANTS

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
            UDP_VARIANT
                // send confirm message
                queueLock(sendingQueue);
                sendConfirm(serverResponse, receiverSendMsgs, progInt, msg_flag_CONFIRM);
                queueUnlock(sendingQueue);
            END_VARIANTS

            // set staye to END
            setProgramState(progInt, fsm_BYE_RECV);
            // signal main to stop waiting
            pthread_cond_signal(progInt->threads->mainCond);
            break;
        case msg_ERR:
            UDP_VARIANT
                // send confirm message
                queueLock(sendingQueue);
                sendConfirm(serverResponse, receiverSendMsgs, progInt, msg_flag_CONFIRM);
                queueUnlock(sendingQueue);
            END_VARIANTS

            // set state to END
            setProgramState(progInt, fsm_END);
            safePrintStderr("ERR FROM %s: %s\n", pBlocks->msg_msg_displayname.start,
                pBlocks->msg_err_MsgContents.start); // TODO:
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

    Buffer* serverResponse = &(progInt->cleanUp->serverResponse);
    bufferResize(serverResponse, 1500); // set max size messages from server

    Buffer* receiverSendMsgs = &(progInt->cleanUp->protocolToSendedByReceiver);
    
    MessageQueue* confirmedMsgs = progInt->cleanUp->confirmedMessages;

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
    bool noerr = true;
    while(getProgramState(progInt) != fsm_END)
    {
        int bytesRx = recvfrom(progInt->netConfig->openedSocket, serverResponse->data,
                                serverResponse->allocated, flags, 
                                progInt->netConfig->serverAddress, 
                                &(progInt->netConfig->serverAddressSize));
        // receiver timeout expired
        if(bytesRx <= 0) {continue;}

        serverResponse->used = bytesRx; //set buffer length (activly used) bytes
        
        UDP_VARIANT
            noerr = disassebleProtocolUDP(serverResponse, &pBlocks, &msgID);
        TCP_VARIANT
            noerr = disassebleProtocolTCP(serverResponse, &pBlocks);
        END_VARIANTS

        #ifdef DEBUG
            debugPrint(stdout, "DEBUG: Receiver:");
            bufferPrint(serverResponse, 1);
            debugPrintSeparator(stdout);
        #endif

        // if in fsm_AUTH state and incoming msg is not REPLY do nothing
        if(getProgramState(progInt) == fsm_START && pBlocks.type != msg_REPLY && pBlocks.type != msg_CONF)
        {
            debugPrint(stdout, "DEBUG: Receiver thrown away a message because"
                "program is in START state and message is not CONFIRM or REPLY");
            continue;
        }


        receiverFSM(progInt, msgID, &pBlocks, progInt->threads->sendingQueue, 
            serverResponse, confirmedMsgs, receiverSendMsgs, noerr);
    }

    debugPrint(stdout, "DEBUG: Receiver ended\n");

    return NULL;
}