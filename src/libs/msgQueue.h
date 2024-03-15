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

#include "customtypes.h"
#include "pthread.h"
#include "buffer.h"

// ----------------------------------------------------------------------------
// Defines, typedefs and structures
// ----------------------------------------------------------------------------

typedef struct Message {
    Buffer* buffer;
    struct Message* behindMe;
    bool confirmed;
    u_int8_t sendCount;
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

/**
 * @brief Return pointer to the first message 
 * 
 * @param queue Queue from which the message will be returned 
 * @return Buffer* First message
 */
Buffer* queueGetMessage(MessageQueue* queue);

/**
 * @brief Adds new message to the queue at the end
 * 
 * @param queue 
 * @param buffer is and input buffer from which the new message will be created
 */
void queueAddMessage(MessageQueue* queue, Buffer* buffer);

/**
 * @brief Deletes first message and moves queue forward
 * 
 * @param queue Queue from which will be the message deleted
 */
void queuePopMessage(MessageQueue* queue);

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

/**
 * @brief Return pointer to the first message but don't unlock queue.
 * 
 * Usage:
 * Buffer* b = queueGetMessageNoUnlock(queue_ptr); 
 * // ... do something with b 
 * queueUnlock(queue_ptr);
 * 
 * @warning queueUnlock() must be called after this function.
 * @param queue Queue from which the message will be returned 
 * @return Message* 
 */
Buffer* queueGetMessageNoUnlock(MessageQueue* queue);

/**
 * @brief Unlock queue
 * 
 * Usage:
 * Buffer* b = queueGetMessageNoUnlock(queue_ptr); 
 * // ... do something with b 
 * queueUnlock(queue_ptr);
 * 
 * @warning DO NOT USE! if you haven't read documentation.
 * 
 * @param queue Queue to be unlocked
 */
void queueUnlock(MessageQueue* queue);

/**
 * @brief Deletes first message and moves queue forward
 * 
 * @warning This function doesn't use mutexes and is not reentrant,
 * so prevention of data corruption is on programmer
 * 
 * @param queue Queue from which will be the message deleted
 */
void queuePopMessageNoMutex(MessageQueue* queue);

#endif
