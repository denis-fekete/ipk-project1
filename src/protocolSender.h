/**
 * @file protocolSender.h
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief Header file for Protocol Sender that sends messages to server 
 * and changes program state accordingly
 * 
 * This module is meant to work in separated thread. 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef PROTOCOL_SENDER_H
#define PROTOCOL_SENDER_H 1

#include "pthread.h"

#include "libs/ipk24protocol.h"

void* protocolSender(void* vargp);

#endif