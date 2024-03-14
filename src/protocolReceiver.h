/**
 * @file protocolReceiver.h
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef PROTOCOL_RECEIVER_H
#define PROTOCOL_RECEIVER_H 1

#include "sys/epoll.h"

#include "libs/customtypes.h"
#include "libs/buffer.h"
#include "libs/utils.h"
#include "libs/networkCom.h"
#include "libs/ipk24protocol.h"
#include "protocolReceiver.h"

void* protocolReceiver(void *vargp);

#endif