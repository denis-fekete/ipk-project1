/**
 * @file customtypes.h
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef CUSTOM_TYPES_H
#define CUSTOM_TYPES_H 1

#include "sys/types.h"

typedef struct BytesBlock {
    char* start; // pointer to the starting character of the block
    size_t len; // length of the block
} BytesBlock;

typedef struct Buffer
{
    char* data;
    size_t allocated ;
    size_t used;
} Buffer;

typedef struct CommunicationDetails {
    Buffer displayName;
    Buffer channelID;
    u_int16_t msgCounter;
} CommunicationDetails;

#endif