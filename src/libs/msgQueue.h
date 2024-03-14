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
#include "buffer.h"

// ----------------------------------------------------------------------------
// Defines, typedefs and structures
// ----------------------------------------------------------------------------

#define DEFAULT_MESSAGE_QUEUE_SIZE 4

typedef struct {
    Buffer msg;
    Buffer* next;
    bool isValid;
} Message;

typedef struct {
    Message* first; // Pointer to the first message
    Message* last; // Pointer to the last message
    Message** queue; // Data array holding messages
    size_t len; // length of queue
    size_t allocated; // allocated space
} MessageQueue;

// ----------------------------------------------------------------------------
// Functions
// ----------------------------------------------------------------------------

/**
 * @brief Initializes MessageQueue with default size (DEFAULT_MESSAGE_QUEUE_SIZE)
 * 
 * @param queue MessageQueue to be initalized
 */
void Queue_Init(MessageQueue* queue);

/**
 * @brief Return pointer to the first message 
 * 
 * @param queue Queue from which the message will be returned 
 * @return Message* 
 */
Message* Queue_GetMessage(MessageQueue* queue);

/**
 * @brief Adds new message to the queue at the end
 * 
 * @param queue 
 * @param buffer is and input buffer from which the new message will be created
 */
void Queue_AddMessage(MessageQueue* queue, Buffer* buffer);

/**
 * @brief Deletes first message and moves queue forward
 * 
 * @param queue Queue from which will be the message deleted
 */
void Queue_PopMessage(MessageQueue* queue);

#endif
