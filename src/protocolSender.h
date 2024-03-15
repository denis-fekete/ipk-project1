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

#include "libs/customtypes.h"
#include "libs/buffer.h"
#include "libs/utils.h"
#include "libs/networkCom.h"
#include "libs/ipk24protocol.h"
#include "libs/msgQueue.h"


void* protocolSender(void* vargp);

#endif