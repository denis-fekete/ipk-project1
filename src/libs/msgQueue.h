/**
 * @file msgQueue.h
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief Header file for MessageQueue for storing and correctly working with 
 * multiple messages
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef MSG_LISH_H
#define MSG_LISH_H

#include "pthread.h"

#include "buffer.h"
#include "utils.h"
#include "ipk24protocol.h"


// ----------------------------------------------------------------------------
// Defines, typedefs and structures
// ----------------------------------------------------------------------------

typedef enum MessageFlags {msg_flag_NONE, msg_flag_DO_NOT_RESEND, msg_flag_AUTH, msg_flag_MSG} msg_flags;

typedef struct Message {
    Buffer* buffer;
    struct Message* behindMe;
    u_int8_t sendCount;
    msg_flags msgFlags;
    bool confirmed;
} Message;

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
 */
void queueAddMessage(MessageQueue* queue, Buffer* buffer, msg_flags msgFlags);

/**
 * @brief Adds new message to the queue at the start
 * 
 * @param queue MessageQueue to which will the new message be added
 * @param buffer is and input buffer from which the new message will be created
 */
void queueAddMessagePriority(MessageQueue* queue, Buffer* buffer, msg_flags msgFlags);

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------


/**
 * @brief Deletes first message and moves queue forward
 * 
 * @param queue Queue from which will be the message deleted
 */
void queuePopMessage(MessageQueue* queue);

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
void queueSetMsgID(MessageQueue* queue, ProgramInterface* progInt);

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

#endif
