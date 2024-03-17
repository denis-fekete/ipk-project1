/**
 * @file msgQueue.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief Implemetation of MessageQueue functionality
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
    while(queue->len > 0)
    {
        queuePopMessage(queue);
    }

    queue->first = NULL;
    queue->last = NULL;
    queue->last = 0;

    pthread_mutex_destroy(&(queue->lock));
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Return pointer to the first message 
 * 
 * @warning This function does not quarantee that the pointer won't be 
 * freed leading undefined behavior or Segmentation Fault. Use 
 * queueGetMessageNoUnlock() instead ... however incorrect use of this
 * function may lead to queue ending in locked state.
 * 
 * @param queue Queue from which the message will be returned 
 * @return Message* 
 */
Message* queueGetMessage(MessageQueue* queue)
{
    THREAD_LOCK;
    Message* message = queueGetMessageNoMutex(queue);
    THREAD_UNLOCK;
    return message;
}

/**
 * @brief Return pointer to the first message 
 * 
 * @warning This function doesn't use mutexes and is not reentrant,
 * so prevention of data corruption is on programmer
 * 
 * @warning This function does not quarantee that the pointer won't be 
 * freed leading undefined behavior or Segmentation Fault. Use 
 * queueGetMessageNoUnlock() instead ... however incorrect use of this
 * function may lead to queue ending in locked state.
 * 
 * @param queue Queue from which the message will be returned 
 * @return Message* 
 */
Message* queueGetMessageNoMutex(MessageQueue* queue)
{
    if(queue->len > 0 && queue->first != NULL)
    {
        return queue->first;
    }

    errHandling("Asking for non-existing message, this is implementation error", 1); // DEBUG: this shouldn't be needed
    return NULL;
}

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
void queueUnlock(MessageQueue* queue)
{
    THREAD_UNLOCK;
}

/**
 * @brief Lock queue mutex
 * @warning DO NOT USE! if you haven't read documentation.
 * 
 * @param queue Queue to be unlocked
 */
void queueLock(MessageQueue* queue)
{
    THREAD_LOCK;
}

#define ALLOCATE_AND_COPY_MSG_AND_BUFFER \
    Message* tmp_msg = (Message*) malloc(sizeof(Message));  \
                                                            \
    if(tmp_msg == NULL)                                     \
    {                                                       \
        errHandling("Malloc failed in queueAddMessage() for Message", 1); /* TODO: change err code */ \
    }                                                       \
                                                            \
    Buffer* tmp_buffer = (Buffer*) malloc(sizeof(Buffer));  \
    if(tmp_msg == NULL)                                     \
    {                                                       \
        errHandling("Malloc failed in queueAddMessage() for Buffer", 1); /* TODO: change err code */ \
    }                                                       \
    /* set default values to the message attributes*/       \
    bufferInit(tmp_buffer);                                 \
    tmp_msg->sendCount = 0;                                 \
                                                            \
    /* Copies input buffer to the new message*/             \
    bufferCopy(tmp_buffer, buffer);                         \
                                                            \
    tmp_msg->buffer = tmp_buffer;                           \
    tmp_msg->msgFlags = msgFlags;                          

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Adds new message to the queue at the end
 * 
 * @param queue MessageQueue to which will the new message be added
 * @param buffer is and input buffer from which the new message will be created
 */
void queueAddMessage(MessageQueue* queue, Buffer* buffer, msg_flags msgFlags)
{

    ALLOCATE_AND_COPY_MSG_AND_BUFFER;

    THREAD_LOCK;
    // if queue doesn't have first, set this msg as first
    if(queue->first == NULL) { queue->first = tmp_msg; }

    // set new message behind last message if exits    
    if(queue->last != NULL) { queue->last->behindMe = tmp_msg; }
    // set new message as last
    queue->last = tmp_msg;

    // set new message's behindMe value to NULL
    queue->last->behindMe = NULL;
    // increase queue size
    queue->len += 1;

    THREAD_UNLOCK;
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Adds new message to the queue at the start
 * 
 * @param queue MessageQueue to which will the new message be added
 * @param buffer is and input buffer from which the new message will be created
 */
void queueAddMessagePriority(MessageQueue* queue, Buffer* buffer, msg_flags msgFlags)
{
    ALLOCATE_AND_COPY_MSG_AND_BUFFER;

    THREAD_LOCK;
    // if queue doesn't have last, set this msg as last
    if(queue->last == NULL) { queue->last = tmp_msg; }

    Message* oldFirst = queue->first;

    // set new message as first
    queue->first = tmp_msg;

    tmp_msg->behindMe = oldFirst;
    
    // increase queue size
    queue->len += 1;

    THREAD_UNLOCK;
}

/**
 * @brief Adds new message to the queue at the start
 * 
 * @warning This function doesn't use mutexes and is not reentrant,
 * so prevention of data corruption is on programmer
 * 
 * @param queue MessageQueue to which will the new message be added
 * @param buffer is and input buffer from which the new message will be created
 */
void queueAddMessagePriorityNoMutex(MessageQueue* queue, Buffer* buffer, msg_flags msgFlags)
{
    ALLOCATE_AND_COPY_MSG_AND_BUFFER;

    // if queue doesn't have last, set this msg as last
    if(queue->last == NULL) { queue->last = tmp_msg; }

    Message* oldFirst = queue->first;

    // set new message as first
    queue->first = tmp_msg;

    tmp_msg->behindMe = oldFirst;
    
    // increase queue size
    queue->len += 1;
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Deletes first message and moves queue forward
 * 
 * @param queue Queue from which will be the message deleted
 */
void queuePopMessage(MessageQueue* queue)
{
    THREAD_LOCK;
    queuePopMessageNoMutex(queue);
    THREAD_UNLOCK
}

/**
 * @brief Deletes first message and moves queue forward
 * 
 * @warning This function doesn't use mutexes and is not reentrant,
 * so prevention of data corruption is on programmer
 * 
 * @param queue Queue from which will be the message deleted
 */
void queuePopMessageNoMutex(MessageQueue* queue)
{
    if(queue->len <= 0)
    {
        return;
    }

    // store old first
    Message* oldFirst = queue->first;
    // set queue first to message behind old first message
    queue->first = oldFirst->behindMe;

    // destroy message
    bufferDestory(oldFirst->buffer); // buffer.data
    free(oldFirst->buffer); // buffer pointer
    free(oldFirst); // message pointer

    // decrease size of queue
    queue->len -= 1;
}

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

    // debugPrint(stdout, "Left it queue: %li\n", queue->len);

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
    THREAD_LOCK;
    size_t currValue = queue->len;
    THREAD_UNLOCK;

    return currValue;
}

/**
 * @brief Adds ONE to sended counter of first message 
 * 
 * @param queue queue to which first message counter will be incremented
 */
void queueMessageSended(MessageQueue* queue)
{
    THREAD_LOCK;
    queue->first->sendCount += 1;
    THREAD_UNLOCK;
}

/**
 * @brief Adds ONE to sended counter of first message 
 * 
 * @param queue queue to which first message counter will be incremented
 */
void queueMessageSendedNoMutex(MessageQueue* queue)
{
    queue->first->sendCount += 1;
}




/**
 * @brief Returns number of times this message was sended 
 * 
 * @param queue queue from which first message will be checked
 * @return u_int8_t number of times first message in queue was sended 
 */
u_int8_t queueGetSendedCounter(MessageQueue* queue)
{
    THREAD_LOCK;
    u_int8_t val = queueGetSendedCounterNoMutex(queue);
    THREAD_UNLOCK;
    return val;
}

/**
 * @brief Returns number of times this message was sended
 * 
 * @warning This function doesn't use mutexes and is not reentrant,
 * so prevention of data corruption is on programmer
 * 
 * @param queue queue from which first message will be checked
 * @return u_int8_t number of times first message in queue was sended 
 */
u_int8_t queueGetSendedCounterNoMutex(MessageQueue* queue)
{
    u_int8_t val = queue->first->sendCount;
    return val;
}

/**
 * @brief Returns value of first message flags
 * 
 * @param queue Queue from which the first message's flags will be returned 
 * @return msg_flags flags to be returned
 */
msg_flags queueGetMessageFlags(MessageQueue* queue)
{
    THREAD_LOCK;
    msg_flags flags = queueGetMessageFlagsNoMutex(queue);
    THREAD_UNLOCK;

    return flags;
}

/**
 * @brief Sets message id of the first message based on program 
 * interface message counter
 * 
 * @param queue Pointer to queue
 * @param progInt Pointer to ProgramInterface based that holds correct message
 * counter
 */
void queueSetMsgID(MessageQueue* queue, ProgramInterface* progInt)
{
    THREAD_LOCK;
    queueSetMsgIDNoMutex(queue, progInt);
    THREAD_UNLOCK;
}

#define HIGHER_BYTE_POSTION 1
#define LOWER_BYTE_POSTION 2
/**
 * @brief Sets message id of the first message based on program 
 * interface message counter
 * 
* @warning This function doesn't use mutexes and is not reentrant,
 * so prevention of data corruption is on programmer
 * 
 * @param queue Pointer to queue
 * @param progInt Pointer to ProgramInterface based that holds correct message
 * counter
 */
void queueSetMsgIDNoMutex(MessageQueue* queue, ProgramInterface* progInt)
{
    if(queue->first != NULL)
    {
        breakMsgIdToBytes(
            &(queue->first->buffer->data[HIGHER_BYTE_POSTION]),
            &(queue->first->buffer->data[LOWER_BYTE_POSTION]),
            progInt->comDetails->msgCounter);
    }
}
#undef HIGHER_BYTE_POSTION
#undef LOWER_BYTE_POSTION


/**
 * @brief Returns value of first message flags
 * 
 * @warning This function doesn't use mutexes and is not reentrant,
 * so prevention of data corruption is on programmer
 * 
 * @param queue Queue from which the first message's flags will be returned 
 * @return msg_flags flags to be returned
 */
msg_flags queueGetMessageFlagsNoMutex(MessageQueue* queue)
{
    return queue->first->msgFlags;
}

#undef THREAD_LOCK
#undef THREAD_UNLOCK