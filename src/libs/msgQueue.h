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

#include "buffer.h"
#include "utils.h"
#include "pthread.h"

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
 * @brief Initializes MessageQueue
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
 * @brief Return pointer to the first message 
 * 
 * @param queue Queue from which the message will be returned 
 * @return Message* First message
 */
Message* queueGetMessage(MessageQueue* queue);

/**
 * @brief Return pointer to the first message 
 * 
 * @warning This function does not work with queue mutex<br>
 * 
 * @warning This function does not quarantee that the pointer won't be 
 * freed leading undefined behavior or Segmentation Fault. Use 
 * queueGetMessageNoUnlock() instead ... however incorrect use of this
 * function may lead to queue ending in locked state.
 * 
 * @param queue Queue from which the message will be returned 
 * @return Message* 
 */
Message* queueGetMessageNoMutex(MessageQueue* queue);

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

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

/**
 * @brief Adds new message to the queue at the start
 * 
 * @warning This function doesn't use mutexes and is not reentrant,
 * so prevention of data corruption is on programmer
 * 
 * @param queue MessageQueue to which will the new message be added
 * @param buffer is and input buffer from which the new message will be created
 */
void queueAddMessagePriorityNoMutex(MessageQueue* queue, Buffer* buffer, msg_flags msgFlags);

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
 * @brief Deletes first message and moves queue forward
 * 
 * @warning This function doesn't use mutexes and is not reentrant,
 * so prevention of data corruption is on programmer
 * 
 * @param queue Queue from which will be the message deleted
 */
void queuePopMessageNoMutex(MessageQueue* queue);

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

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Adds ONE to sended counter of first message 
 * 
 * @param queue queue to which first message counter will be incremented
 */
void queueMessageSended(MessageQueue* queue);

/**
 * @brief Adds ONE to sended counter of first message 
 * 
 * @warning This function doesn't use mutexes and is not reentrant,
 * so prevention of data corruption is on programmer
 * 
 * @param queue queue to which first message counter will be incremented
 */
void queueMessageSendedNoMutex(MessageQueue* queue);

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Returns number of times this message was sended 
 * 
 * @param queue queue from which first message will be checked
 * @return u_int8_t number of times first message in queue was sended 
 */
u_int8_t queueGetSendedCounter(MessageQueue* queue);

/**
 * @brief Returns number of times this message was sended
 * 
 * @warning This function doesn't use mutexes and is not reentrant,
 * so prevention of data corruption is on programmer
 * 
 * @param queue queue from which first message will be checked
 * @return u_int8_t number of times first message in queue was sended 
 */
u_int8_t queueGetSendedCounterNoMutex(MessageQueue* queue);

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
 * @brief Returns value of first message flags
 * 
 * @warning This function doesn't use mutexes and is not reentrant,
 * so prevention of data corruption is on programmer
 * 
 * @param queue Queue from which the first message's flags will be returned 
 * @return msg_flags flags to be returned
 */
msg_flags queueGetMessageFlagsNoMutex(MessageQueue* queue);

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Unlock queue mutex
 * 
 * @warning DO NOT USE! if you haven't read documentation.
 * 
 * @param queue Queue to be unlocked
 */
void queueUnlock(MessageQueue* queue);

/**
 * @brief Lock queue mutex
 * @warning DO NOT USE! if you haven't read documentation.
 * 
 * @param queue Queue to be unlocked
 */
void queueLock(MessageQueue* queue);

#endif
