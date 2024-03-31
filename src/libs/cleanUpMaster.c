/**
 * @file cleanUpMaster.h
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief Implementation of functions for Program Interface 
 * initializing and destroying, as well as function for handling SIGINT
 * singals.
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "cleanUpMaster.h"

extern ProgramInterface* globalProgInt;

#define IF_NULL_ERR(ptr, message, errCode)  \
    if(ptr == NULL) {                       \
        errHandling(message, errCode);      \
    }

/**
 * @brief Initializes allocated structures in program interface
 * 
 */
void programInterfaceInit(ProgramInterface* pI)
{
    // ------------------------------------------------------------------------
    // ThreadCommunication
    // ------------------------------------------------------------------------

    ThreadCommunication* threads = (ThreadCommunication*) malloc(sizeof(ThreadCommunication));
    IF_NULL_ERR(threads, "Failed to allocate memory for ThreadCommunication", 1); // TODO:err code

    threads->fsmState = fsm_START;
    pI->threads = threads;
    //-------------------------------------------------------------------------
    // queue of outcoming (user sent) messages
    MessageQueue* sendingQueue = (MessageQueue*) malloc(sizeof(MessageQueue));
    IF_NULL_ERR(sendingQueue, "Failed to allocate memory for sending MessageQueue", 1); // TODO:err code
    queueInit(sendingQueue); 

    pI->threads->sendingQueue = sendingQueue;
    //-------------------------------------------------------------------------
    // initialize mutexes and conditions for thread communication
    
    pthread_cond_t* conditions[3]; 
    for(short i = 0; i < 3; i++)
    {
       conditions[i] = (pthread_cond_t*) malloc(sizeof(pthread_cond_t));
    }
    IF_NULL_ERR(conditions, "Failed to allocate memory for thread conditions", 1); // TODO:err code

    pthread_cond_init(conditions[0], NULL);
    pthread_cond_init(conditions[1], NULL);
    pthread_cond_init(conditions[2], NULL);

    pthread_mutex_t* mutexes[5];
    for(short i = 0; i < 5; i++)
    {
        mutexes[i] = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
        IF_NULL_ERR(mutexes, "Failed to allocate memory for thread mutexes", 1); // TODO:err code
    }

    pthread_mutex_init(mutexes[0], NULL);
    pthread_mutex_init(mutexes[1], NULL);
    pthread_mutex_init(mutexes[2], NULL);
    pthread_mutex_init(mutexes[3], NULL);
    pthread_mutex_init(mutexes[4], NULL);


    pI->threads->senderEmptyQueueCond = conditions[0];
    pI->threads->senderEmptyQueueMutex = mutexes[0];

    pI->threads->rec2SenderCond = conditions[1];
    pI->threads->rec2SenderMutex = mutexes[1];

    pI->threads->mainCond = conditions[2];
    pI->threads->mainMutex = mutexes[2];

    pI->threads->fsmMutex = mutexes[3];
    pI->threads->stdoutMutex = mutexes[4];

    //-------------------------------------------------------------------------
    // NetworkConfig
    //-------------------------------------------------------------------------

    NetworkConfig* netConfig = (NetworkConfig*) malloc(sizeof(NetworkConfig));
    IF_NULL_ERR(netConfig, "Failed to allocate memory for network configuration", 1); // TODO:err code

    pI->netConfig = netConfig;
    
    //-------------------------------------------------------------------------
    // CommunicationDetails
    //-------------------------------------------------------------------------

    CommunicationDetails* comDetails = (CommunicationDetails*) malloc(sizeof(CommunicationDetails));
    IF_NULL_ERR(comDetails, "Failed to allocate memory for Communication Details", 1); // TODO:err code

    comDetails->msgCounter = 0;
    // initalize buffers
    bufferInit(&(comDetails->displayName));
    bufferInit(&(comDetails->channelID));

    pI->comDetails = comDetails;
    
    //-------------------------------------------------------------------------
    // CleanUp
    //-------------------------------------------------------------------------
    CleanUp* cleanUp = (CleanUp*) malloc(sizeof(CleanUp));
    IF_NULL_ERR(comDetails, "Failed to allocate memory for CleanUp", 1); // TODO:err code

    bufferInit(&(cleanUp->clientInput));
    bufferInit(&(cleanUp->protocolToSendedByMain));
    bufferInit(&(cleanUp->protocolToSendedByReceiver));
    bufferInit(&(cleanUp->serverResponse));

    MessageQueue* confirmedMessages = (MessageQueue*) malloc(sizeof(MessageQueue));
    IF_NULL_ERR(comDetails, "Failed to allocate memory for CleanUp.MessageQueue", 1); // TODO:err code
    queueInit(confirmedMessages);

    cleanUp->confirmedMessages = confirmedMessages;

    pI->cleanUp = cleanUp;
}

/**
 * @brief Destorys and frees allocated structures inside global program
 * interface
 * 
 */
void programInterfaceDestroy(ProgramInterface* pI)
{
    //-------------------------------------------------------------------------
    // CleanUp
    //-------------------------------------------------------------------------

    bufferDestroy(&(pI->cleanUp->clientInput));
    bufferDestroy(&(pI->cleanUp->protocolToSendedByMain));
    bufferDestroy(&(pI->cleanUp->protocolToSendedByReceiver));
    bufferDestroy(&(pI->cleanUp->serverResponse));
    queueDestroy(pI->cleanUp->confirmedMessages);
    free(pI->cleanUp);

    //-------------------------------------------------------------------------
    // CommunicationDetails
    //-------------------------------------------------------------------------

    bufferDestroy(&(pI->comDetails->displayName));
    bufferDestroy(&(pI->comDetails->channelID));
    free(pI->comDetails);

    //-------------------------------------------------------------------------
    // NetworkConfig
    //-------------------------------------------------------------------------

    free(pI->netConfig);

    // ------------------------------------------------------------------------
    // ThreadCommunication
    // ------------------------------------------------------------------------
    
    pthread_mutex_destroy(pI->threads->fsmMutex);
    pthread_mutex_destroy(pI->threads->stdoutMutex);
    free(pI->threads->fsmMutex);
    free(pI->threads->stdoutMutex);

    pthread_mutex_destroy(pI->threads->mainMutex);
    pthread_cond_destroy(pI->threads->mainCond);
    free(pI->threads->mainMutex);
    free(pI->threads->mainCond);

    pthread_mutex_destroy(pI->threads->rec2SenderMutex);
    pthread_cond_destroy(pI->threads->rec2SenderCond);
    free(pI->threads->rec2SenderMutex);
    free(pI->threads->rec2SenderCond);

    pthread_mutex_destroy(pI->threads->senderEmptyQueueMutex);
    pthread_cond_destroy(pI->threads->senderEmptyQueueCond);
    free(pI->threads->senderEmptyQueueMutex);
    free(pI->threads->senderEmptyQueueCond);

    queueDestroy(pI->threads->sendingQueue);

    free(pI->threads);

    // ------------------------------------------------------------------------
    free(pI);
}

/**
 * @brief Function that handles SIGINT signals and correctly
 * frees up all memory and closes add comminication 
 * 
 * @param num 
 */
void sigintHandler(int num) {
    if(num){} // anti-error--compiler

    // set program state to SIGINT_BYE, which will lead to main thread to exit
    setProgramState(globalProgInt, fsm_SIGINT_BYE);
    // signal main thread to wake up if suspended
    pthread_cond_signal(globalProgInt->threads->mainCond);

    queueLock(globalProgInt->threads->sendingQueue);
    queuePopAllMessages(globalProgInt->threads->sendingQueue);
    queueUnlock(globalProgInt->threads->sendingQueue);

    // send bye to the server
    sendBye(globalProgInt);

    // wait on mainMutex, sender will singal that it sended last BYE and exited
    pthread_cond_wait(globalProgInt->threads->mainCond, globalProgInt->threads->mainMutex);

    // close socket
    shutdown(globalProgInt->netConfig->openedSocket, SHUT_RDWR);
    // destory program interface
    programInterfaceDestroy(globalProgInt);
    exit(0);
}
