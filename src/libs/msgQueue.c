/**
 * @file msgQueue.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "msgQueue.h"

/**
 * @brief Initializes MessageQueue with default size (DEFAULT_MESSAGE_QUEUE_SIZE)
 * 
 * @param queue MessageQueue to be initalized
 */
void Queue_Init(MessageQueue* queue)
{
    queue->queue = (Message**) malloc(sizeof(Message*) * DEFAULT_MESSAGE_QUEUE_SIZE);
    if(queue->queue == NULL)
    {
        errHandling("Allocating memory for MessageQueue in queueInit() failed", 1); //TODO: change err code
    }

    queue->first = NULL;
    queue->last = NULL;
    queue->len = 0;
    queue->allocated = DEFAULT_MESSAGE_QUEUE_SIZE;
}

/**
 * @brief Return pointer to the first message 
 * 
 * @param queue Queue from which the message will be returned 
 * @return Message* 
 */
Message* Queue_GetMessage(MessageQueue* queue)
{
    if(queue->len > 0 || queue->first != NULL)
    {
        return queue->first;
    }
}

/**
 * @brief Adds new message to the queue at the end
 * 
 * @param queue 
 * @param buffer is and input buffer from which the new message will be created
 */
void Queue_AddMessage(MessageQueue* queue, Buffer* buffer)
{
    // Check if queue needs resizing
    if(queue->len >= queue->allocated)
    {
            
    }
}

/**
 * @brief Deletes first message and moves queue forward
 * 
 * @param queue Queue from which will be the message deleted
 */
void Queue_PopMessage(MessageQueue* queue)
{

}