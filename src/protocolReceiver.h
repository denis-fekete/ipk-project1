#ifndef PROTOCOL_RECEIVER_H
#define PROTOCOL_RECEIVER_H 1

#include "customtypes.h"
#include "customtypes.h"
#include "buffer.h"
#include "utils.h"
#include "networkCom.h"
#include "ipk24protocol.h"
#include "protocolReceiver.h"
#include "sys/epoll.h"

void* protocolReceiver(void *vargp);

#endif