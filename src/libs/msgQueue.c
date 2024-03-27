/**
 * @file msgQueue.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief Implemetation of MessageQueue functionality
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "msgQueue.h"

#define HIGHER_MSGID_BYTE_POSTION 2
#define LOWER_MSGID_BYTE_POSTION 1
#define CMD_BYTE_POSITION 0

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
    queuePopAllMessages(queue);

    pthread_mutex_destroy(&(queue->lock));
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Unlock queue mutex
 * 
 * @param queue Queue to be unlocked
 */
void queueUnlock(MessageQueue* queue)
{
    pthread_mutex_unlock(&(queue->lock));
}

/**
 * @brief Lock queue mutex
 * 
 * @param queue Queue to be unlocked
 */
void queueLock(MessageQueue* queue)
{
    pthread_mutex_lock(&(queue->lock));
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Return pointer to the first message 
 * 
 * @param queue Queue from which the message will be returned 
 * @return Message* 
 */
Message* queueGetMessage(MessageQueue* queue)
{
    if(queue->len > 0 && queue->first != NULL)
    {
        return queue->first;
    }

    return NULL;
}

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
Message* createMessage(Buffer* buffer, msg_flags msgFlags)
{
    Message* tmpMsg = (Message*) malloc(sizeof(Message));

    if(tmpMsg == NULL)
    {
        errHandling("Malloc failed in queueAddMessage() for Message", 1); /* TODO: change err code */
    }

    Buffer* tmpBuffer = (Buffer*) malloc(sizeof(Buffer));
    if(tmpMsg == NULL)
    {
        errHandling("Malloc failed in queueAddMessage() for Buffer", 1); /* TODO: change err code */
    }
    /* set default values to the message attributes*/
    bufferInit(tmpBuffer);

    /* Copies input buffer to the new message*/
    bufferCopy(tmpBuffer, buffer);

    tmpMsg->sendCount = 0;
    tmpMsg->confirmed = false;
    tmpMsg->buffer = tmpBuffer;
    tmpMsg->msgFlags = msgFlags;

    return tmpMsg;
}

/**
 * @brief Adds new message to the queue at the end
 * 
 * @param queue MessageQueue to which will the new message be added
 * @param buffer is and input buffer from which the new message will be created
 */
void queueAddMessage(MessageQueue* queue, Buffer* buffer, msg_flags msgFlags)
{
    Message* newMessage = createMessage(buffer, msgFlags);

    // if queue doesn't have first, set this msg as first
    if(queue->first == NULL) { queue->first = newMessage; }

    // set new message behind last message if exits    
    if(queue->last != NULL) { queue->last->behindMe = newMessage; }
    // set new message as last
    queue->last = newMessage;

    // set new message's behindMe value to NULL
    queue->last->behindMe = NULL;
    // increase queue size
    queue->len += 1;
}

/**
 * @brief Adds new message to the queue at the start
 * 
 * @param queue MessageQueue to which will the new message be added
 * @param buffer is and input buffer from which the new message will be created
 */
void queueAddMessagePriority(MessageQueue* queue, Buffer* buffer, msg_flags msgFlags)
{
    Message* newMessage = createMessage(buffer, msgFlags);

    // if queue doesn't have last, set this msg as last
    if(queue->last == NULL) { queue->last = newMessage; }

    Message* oldFirst = queue->first;

    // set new message as first
    queue->first = newMessage;

    newMessage->behindMe = oldFirst;
    
    // increase queue size
    queue->len += 1;
}

/**
 * @brief Adds message to queue at the start. Added emssage won't contain buffer, only
 * message ID
 * 
 * @param queue Pointer to the queue 
 * @param buffer Buffer where message is stored
 */
void queueAddMessageOnlyID(MessageQueue* queue, Buffer* buffer)
{
    Message* tmpMsg = (Message*) malloc(sizeof(Message));

    if(tmpMsg == NULL)
    {
        errHandling("Malloc failed in queueAddMessageOnlyID() for Message", 1); /* TODO: change err code */
    }

    tmpMsg->buffer = NULL;
    tmpMsg->msgFlags = msg_flag_ERR;

    if(buffer->used < 3) 
    { 
        errHandling("Buffer provided in queueAddMessageOnlyID"
        "is shorter than 3 bytes!", 1); /*TODO: change*/ 
    }
    
    tmpMsg->highMsgId = buffer->data[HIGHER_MSGID_BYTE_POSTION];
    tmpMsg->lowMsgId = buffer->data[LOWER_MSGID_BYTE_POSTION];

    // if queue doesn't have last, set this msg as last
    if(queue->last == NULL) { queue->last = tmpMsg; }

    Message* oldFirst = queue->first;

    // set new message as first
    queue->first = tmpMsg;

    tmpMsg->behindMe = oldFirst;
    
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
    if(queue->len <= 0)
    {
        return;
    }

    // store old first
    Message* oldFirst = queue->first;
    // set queue first to message behind old first message
    queue->first = oldFirst->behindMe;

    if(oldFirst == queue->last)
    {
        queue->last = NULL;
    }

    if(oldFirst->buffer != NULL)
    {
        // destroy message
        bufferDestory(oldFirst->buffer); // buffer.data
        free(oldFirst->buffer); // buffer pointer
    }
    free(oldFirst); // message pointer

    // decrease size of queue
    queue->len -= 1;
}

/**
 * @brief Deletes all messages in queue
 * 
 * @param queue Queue from which will the messages be deleted
 */
void queuePopAllMessages(MessageQueue* queue)
{
    while(queue->len > 0)
    {
        queuePopMessage(queue);
    }

    queue->first = NULL;
    queue->last = NULL;
    queue->last = 0;
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
    
    if(queue->len == 0){
        return true;
    }

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
    if(queue == NULL) 
    {
        errHandling("In queueIsEmpty(), uninitialized queue was passed as argument", 1); //TODO:
    }

    size_t currValue = queue->len;

    return currValue;
}

/**
 * @brief Looks for message ID in queue, works only with message queues
 *  without buffers containing only msg ids
 * 
 * @param queue Pointer to the queue
 * @param highB Higer byte to compare to
 * @param lowB Lower byte to compare to
 * @return true Message with this ID was found
 * @return false Message with this ID was not found
 */
bool queueContainsMessageId(MessageQueue* queue, char highB, char lowB)
{
    if(queue == NULL) { return false; }

    Message* msg = queue->first;
    while(msg != NULL)
    {
        if(highB == msg->highMsgId)
        {
            if(lowB == msg->lowMsgId) { return true; }
        }

        msg = msg->behindMe;
    }
    
    return false;
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------


/**
 * @brief Adds ONE to sended counter of first message 
 * 
 * @param queue queue to which first message counter will be incremented
 */
void queueMessageSended(MessageQueue* queue)
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
    return queue->first->sendCount;
}

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
void queueSetMessageID(MessageQueue* queue, ProgramInterface* progInt)
{
    // check if first exists and if message is not confirm
    if(queue->first != NULL && queue->first->msgFlags != msg_flag_CONFIRM)
    {
        // break msgCounter into bytes and store it into buffer at positions
        breakU16IntToBytes(
            &(queue->first->buffer->data[HIGHER_MSGID_BYTE_POSTION]),
            &(queue->first->buffer->data[LOWER_MSGID_BYTE_POSTION]),
            progInt->comDetails->msgCounter);
    }
}

/**
 * @brief Returns message ID of the first message from queue
 * 
 * @param queue Pointer to queue
 * @return uint16_t 
 */
uint16_t queueGetMessageID(MessageQueue* queue)
{
    if(queue->first != NULL)
    {
        return convert2BytesToU16Int(queue->first->buffer->data[LOWER_MSGID_BYTE_POSTION],
            queue->first->buffer->data[HIGHER_MSGID_BYTE_POSTION]);
    }

    return 0;
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Returns value of first message flags
 * 
 * @param queue Queue from which the first message's flags will be returned 
 * @return msg_flags flags to be returned
 */
msg_flags queueGetMessageFlags(MessageQueue* queue)
{
    return queue->first->msgFlags;
}

/**
 * @brief Sets flag of the first message in queue
 * 
 * @param queue Queue from which the first message's flags will be changed 
 * @param newFlag New message of type msg_flags 
 */
void queueSetMessageFlags(MessageQueue* queue, msg_flags newFlag)
{
    if(queue->first != NULL)
    {
        queue->first->msgFlags = newFlag;
        return;
    }

    errHandling("Unexpected calling of message flags on empty MessageQueue", 1); //TODO: change
}

/**
 * @brief Returns MessageType (msg_t) of the first message in queue
 * 
 * @param queue Pointer to the queue
 * @return cmd_t Detected value
 */
msg_t queueGetMessageMsgType(MessageQueue* queue)
{
    return ( (enum MessageType) queue->first->buffer->data[CMD_BYTE_POSITION] );
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------


/**
 * @brief Converts integer into and MessageType
 * 
 * @param input Input integer
 * @return msg_t MessageType to be returned
 */
msg_t uchar2msgType(unsigned char input)
{
    switch (input)
    {
    case 0x00: return msg_CONF;
    case 0x01: return msg_REPLY;
    case 0x02: return msg_AUTH;
    case 0x03: return msg_JOIN;
    case 0x04: return msg_MSG;
    case 0xFE: return msg_ERR;
    case 0xFF: return msg_BYE;
    default: return msg_UNKNOWN;
    }
}

#undef HIGHER_BYTE_POSITION
#undef LOWER_BYTE_POSITION
#undef CMD_BYTE_POSITION