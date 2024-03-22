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
#include "networkCom.h"

struct Buffer;
struct BytesBlock;
struct NetworkConfig;
struct CommunicationDetails;
struct ThreadCommunication;
struct ProgramInterface;
struct Message;
struct MessageQueue;


typedef struct Buffer Buffer; 
typedef struct BytesBlock BytesBlock;
typedef struct NetworkConfig NetworkConfig; 
typedef struct CommunicationDetails CommunicationDetails; 
typedef struct ThreadCommunication ThreadCommunication;
typedef struct ProgramInterface ProgramInterface; 
typedef struct Message Message;
typedef struct MessageQueue MessageQueue;

typedef enum FSM fsm_t;
typedef enum CommandType cmd_t;
typedef enum Protocols prot_t;
typedef enum MessageType msg_t;

typedef enum MessageFlags {
    msg_flag_NONE, 
    msg_flag_DO_NOT_RESEND, 
    msg_flag_AUTH, 
    msg_flag_REJECTED, /*auth was rejected*/
    msg_flag_CONFIRMED, /*auth was confirmed*/
    msg_flag_ERR
    } msg_flags;


#include "msgQueue.h"


#endif