/**
 * @file msgQueue.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "msgQueue.h"

#define THREAD_LOCK pthread_mutex_lock(&(queue->lock));
#define THREAD_UNLOCK pthread_mutex_unlock(&(queue->lock));

/**
 * @brief Initializes MessageQueue with default size (DEFAULT_MESSAGE_QUEUE_SIZE)
 * 
 * @param queue MessageQueue to be initalized
 */
void queueInit(MessageQueue* queue)
{
    queue->first = NULL;
    queue->last = NULL;
    queue->len = 0;

    pthread_mutex_init(&(queue->lock), NULL);
}

/**
 * @brief Destroys MessageQueue
 * 
 * @param queue queue to be destroyed
 */
void queueDestroy(MessageQueue* queue)
{
    for(unsigned i = 0; i < queue->len; i++)
    {
        queuePopMessage(queue);
    }

    queue->first = NULL;
    queue->last = NULL;
    queue->last = 0;

    pthread_mutex_destroy(&(queue->lock));
}

/**
 * @brief Return pointer to the first message 
 * 
 * @param queue Queue from which the message will be returned 
 * @return Message* 
 */
Buffer* queueGetMessage(MessageQueue* queue)
{
    THREAD_LOCK;
    if(queue->len > 0 && queue->first != NULL)
    {
        THREAD_UNLOCK;
        return queue->first->buffer;
    }

    THREAD_UNLOCK;
    errHandling("Asking for non-existing message, this is implementation error", 1); // DEBUG: this shouldn't be needed
    return NULL;
}

/**
 * @brief Adds new message to the queue at the end
 * 
 * @param queue 
 * @param buffer is and input buffer from which the new message will be created
 */
void queueAddMessage(MessageQueue* queue, Buffer* buffer)
{
    THREAD_LOCK;

    Message* tmp_msg = (Message*) malloc(sizeof(Message));

    if(tmp_msg == NULL)
    {
        THREAD_UNLOCK;
        errHandling("Malloc failed in queueAddMessage() for Message", 1); // TODO: change err code
    }

    Buffer* tmp_buffer = (Buffer*) malloc(sizeof(Buffer));
    if(tmp_msg == NULL)
    {
        THREAD_UNLOCK;
        errHandling("Malloc failed in queueAddMessage() for Buffer", 1); // TODO: change err code
    }

    bufferInit(tmp_buffer);

    // Copies input buffer to the new message
    bufferCopy(tmp_buffer, buffer);

    tmp_msg->buffer = tmp_buffer;

    if(queue->last != NULL) 
    {
        // set new message behind last message from queue    
        queue->last->behindMe = tmp_msg;
    }

    if(queue->first == NULL)
    {
        queue->first = tmp_msg;
    }

    // set new message as last
    queue->last = tmp_msg;
    // set new message's behindMe message to NULL
    queue->last->behindMe = NULL;
    // increase queue size
    queue->len += 1;

    THREAD_UNLOCK;
}

/**
 * @brief Deletes first message and moves queue forward
 * 
 * @param queue Queue from which will be the message deleted
 */
void queuePopMessage(MessageQueue* queue)
{
    THREAD_LOCK;

    if(queue->len <= 0)
    {
        THREAD_UNLOCK
        return;
    }

    // store old first
    Message* oldFirst = queue->first;
    // set queue first to message behind old first message
    queue->first = oldFirst->behindMe;

    // destroy message
    // bufferDestory(oldFirst->buffer);
    // free(oldFirst);

    // decrease size of queue
    queue->len -= 1;

    THREAD_UNLOCK
}

/**
 * @brief Check if MessageQueue is empty
 * 
 * @param queue Queue to be checked
 * @return true if queue is empty
 * @return false if queue is not empty
 */
bool queueIsEmpty(MessageQueue* queue)
{
    if(queue == NULL) 
    {
        errHandling("In queueIsEmpty(), uninitialized queue was passed as argument", 1); //TODO:
    }

    THREAD_LOCK;
    if(queue->len == 0){
        THREAD_UNLOCK;
        return true;
    }

    THREAD_UNLOCK;
    return false;
}

/**
 * @brief Returns length of MesssageQueue
 * 
 * @param queue queue to be checked
 * @return size_t Number of Messages in queue
 */
size_t queueLength(MessageQueue* queue)
{
    THREAD_LOCK
    size_t currValue = queue->len;
    THREAD_UNLOCK

    return currValue;
}

#undef THREAD_LOCK
#undef THREAD_UNLOCK