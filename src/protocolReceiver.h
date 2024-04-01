/**
 * @file protocolReceiver.h
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief Header file for Protocol Receiver that receives messages from
 * server and changes program state accordingly
 * 
 * This module is meant to work in separated thread
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef PROTOCOL_RECEIVER_H
#define PROTOCOL_RECEIVER_H 1

#include "sys/epoll.h"

#include "libs/ipk24protocol.h"

/**
 * @brief Create err protocol
 * 
 * @param receiverSendMsgs Buffer to which should message be stored
 * @param progInt Pointer to Program Interface
 * @param message Message to be sended to the server
 */
void sendError(Buffer* receiverSendMsgs, ProgramInterface* progInt, const char* message);

/**
 * @brief Create confirm protocol
 * 
 * @param serverResponse Buffer from which will referenceID be taken
 * @param receiverSendMsgs Buffer to which should message be stored
 * @param progInt Pointer to Program Interface
 * @param flags Flags to be added to the message
 */
void sendConfirm(Buffer* serverResponse, Buffer* receiverSendMsgs, ProgramInterface* progInt, msg_flags flags);

/**
 * @brief Creates BYE message and sends it to server
 * 
 * @param progInt Pointer to program interface
 */
void sendBye(ProgramInterface* progInt);

/**
 * @brief Initializes protocol receiving functionality 
 * 
 * @param vargp arguments
 * @return void* 
 */
void* protocolReceiver(void *vargp);

#endif