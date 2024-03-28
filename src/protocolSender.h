/**
 * @file protocolSender.h
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
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