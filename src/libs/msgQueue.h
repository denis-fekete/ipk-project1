/**
 * @file msgQueue.h
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief Declaration of fucntions and structures of MessageQueue for storing 
 * and correctly working with multiple messages.
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef MSG_LISH_H
#define MSG_LISH_H

#include "pthread.h"

#include "programInterface.h"

// ----------------------------------------------------------------------------
// Defines, typedefs and structures
// ----------------------------------------------------------------------------

typedef struct ProgramInterface ProgramInterface; // declare that ProgramInterface will exist

/**
 * @brief Flags that can be added to the send messages 
 */
typedef enum MessageFlags {
    msg_flag_NONE, /*default*/
    msg_flag_DO_NOT_RESEND, /*do not resend*/
    msg_flag_NOK_REPLY, /*reply is not okey*/
    msg_flag_AUTH, /*authentication message*/
    msg_flag_REJECTED, /*auth was rejected*/
    msg_flag_CONFIRMED, /*auth was confirmed*/
    msg_flag_ERR, /*error / unknown message flag*/
    msg_flag_CONFIRM, /*if message is confirm*/
    msg_flag_BYE /*if message is bye*/
    } msg_flags;

/**
 * @brief Type of messages
 */
typedef enum MessageType {
    msg_CONF = 0x00, /*confirmation, UDP only*/
    msg_REPLY = 0x01, /*reply message*/
    msg_AUTH = 0x02, /*authentication*/
    msg_JOIN = 0x03, /*join*/
    msg_MSG = 0x04, /*plain message*/
    msg_ERR = 0xFE, /*error message*/
    msg_BYE = 0xFF, /*bye message*/
    msg_UNKNOWN = 0xAA, /*unknown message type*/
    msg_CORRUPTED = 0xAB /*partialy okey but corrupted msg*/
  } msg_t;

/**
 * @brief Mesage in list containing Buffer with message contents,
 *  type of message, flag and pointer to the message behind this message
 */
typedef struct Message {
    Buffer* buffer;
    struct Message* behindMe;
    
    union 
    {
        u_int8_t sendCount;
        unsigned char highMsgId; 
    };
    union 
    {
        bool confirmed;
        unsigned char lowMsgId;
    };
    
    unsigned char type;
    msg_flags msgFlags;
} Message;

/**
 * @brief MessageQueue is priority FIFO (first in first out) queue containg 
 * Message structures and mutex lock for protecting data from being access 
 * at multiple points in same time.
 */
typedef struct MessageQueue {
    Message* first; // Pointer to the first message
    Message* last; // Pointer to the last message
    size_t len; // length of queue
    pthread_mutex_t lock;
} MessageQueue;

// ----------------------------------------------------------------------------
// Functions
// ----------------------------------------------------------------------------

/**
 * @brief Initializes MessageQueue with default size (DEFAULT_MESSAGE_QUEUE_SIZE)
 * 
 * @param queue MessageQueue to be initalized
 */
void queueInit(MessageQueue* queue);

/**
 * @brief Destroys MessageQueue
 * 
 * @param queue queue to be destroyed
 */
void queueDestroy(MessageQueue* queue);

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Unlock queue mutex
 * 
 * @param queue Queue to be unlocked
 */
void queueUnlock(MessageQueue* queue);

/**
 * @brief Lock queue mutex
 * 
 * @param queue Queue to be unlocked
 */
void queueLock(MessageQueue* queue);

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Return pointer to the first message 
 * 
 * @param queue Queue from which the message will be returned 
 * @return Message* 
 */
Message* queueGetMessage(MessageQueue* queue);

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Creates and initializes message and returns pointer to it
 * 
 * @param buffer Contents of buffer that will be copied into message
 * @param msgFlags Flags that will be set
 * @return Message* Pointer to new allocated message
 */
Message* createMessage(Buffer* buffer, msg_flags msgFlags);

/**
 * @brief Adds new message to the queue at the end
 * 
 * @param queue MessageQueue to which will the new message be added
 * @param buffer is and input buffer from which the new message will be created
 * @param cmdType type of command to be set to the message
 */
void queueAddMessage(MessageQueue* queue, Buffer* buffer, msg_flags msgFlags, unsigned char cmdType);

/**
 * @brief Adds new message to the queue at the start
 * 
 * @param queue MessageQueue to which will the new message be added
 * @param buffer is and input buffer from which the new message will be created
 */
void queueAddMessagePriority(MessageQueue* queue, Buffer* buffer, msg_flags msgFlags);


/**
 * @brief Adds message to queue at the start. Added emssage won't contain buffer, only
 * message ID
 * 
 * @param queue Pointer to the queue 
 * @param buffer Buffer where message is stored
 */
void queueAddMessageOnlyID(MessageQueue* queue, Buffer* buffer);

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------


/**
 * @brief Deletes first message and moves queue forward
 * 
 * @param queue Queue from which will be the message deleted
 */
void queuePopMessage(MessageQueue* queue);

/**
 * @brief Deletes all messages in queue
 * 
 * @param queue Queue from which will the messages be deleted
 */
void queuePopAllMessages(MessageQueue* queue);

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Check if MessageQueue is empty
 * 
 * @param queue Queue to be checked
 * @return true if queue is empty
 * @return false if queue is not empty
 */
bool queueIsEmpty(MessageQueue* queue);

/**
 * @brief Returns length of MesssageQueue
 * 
 * @param queue queue to be checked
 * @return size_t Number of Messages in queue
 */
size_t queueLength(MessageQueue* queue);

/**
 * @brief Looks for message ID in queue, works only with message queues
 *  without buffers containing only msg ids
 * 
 * @param queue Pointer to the queue
 * @param incoming Incoming message pointer
 * @return true Message with this ID was found
 * @return false Message with this ID was not found
 */
bool queueContainsMessageId(MessageQueue* queue, Message* incoming);

/**
 * @brief Adds ONE to sended counter of first message 
 * 
 * @param queue queue to which first message counter will be incremented
 */
void queueMessageSended(MessageQueue* queue);

/**
 * @brief Returns number of times this message was sended
 * 
 * @param queue queue from which first message will be checked
 * @return u_int8_t number of times first message in queue was sended 
 */
u_int8_t queueGetSendedCounter(MessageQueue* queue);

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Sets message id of the first message based on program 
 * interface message counter
 * 
 * @param queue Pointer to queue
 * @param progInt Pointer to ProgramInterface based that holds correct message
 * counter
 */
void queueSetMessageID(MessageQueue* queue, ProgramInterface* progInt);

/**
 * @brief Returns message ID of the first message from queue
 * 
 * @param queue Pointer to queue
 * @return uint16_t 
 */
uint16_t queueGetMessageID(MessageQueue* queue);

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Returns value of first message flags
 * 
 * @param queue Queue from which the first message's flags will be returned 
 * @return msg_flags flags to be returned
 */
msg_flags queueGetMessageFlags(MessageQueue* queue);

/**
 * @brief Sets flag of the first message in queue
 * 
 * @param queue Queue from which the first message's flags will be changed 
 * @param newFlag New message of type msg_flags 
 */
void queueSetMessageFlags(MessageQueue* queue, msg_flags newFlag);

/**
 * @brief Returns MessageType (msg_t) of the first message in queue
 * 
 * @param queue Pointer to the queue
 * @return msg_t Detected value
 */
msg_t queueGetMessageMsgType(MessageQueue* queue);

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------


/**
 * @brief Converts integer into and MessageType
 * 
 * @param input Input integer
 * @return msg_t MessageType to be returned
 */
msg_t uchar2msgType(unsigned char input);

#endif /*MSG_LISH_H*/
