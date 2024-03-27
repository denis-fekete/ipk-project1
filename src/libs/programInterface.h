#ifndef PROGRAM_INTERFACE_H
#define PROGRAM_INTERFACE_H

#include "networkCom.h"
#include "buffer.h"

// ----------------------------------------------------------------------------
// Structures
// ----------------------------------------------------------------------------

typedef struct CommunicationDetails {
    Buffer displayName;
    Buffer channelID;
    uint16_t msgCounter;
} CommunicationDetails;

typedef struct ThreadCommunication {
    // true = work as normal, false = prepare to end
    bool continueProgram; // variable to signal that program should prepare for finishing
    fsm_t fsmState; // state of FSM, how should program behave 

    pthread_mutex_t* stdoutMutex;
    pthread_mutex_t* fsmMutex;

    struct MessageQueue* sendingQueue; // queue of outcoming (user sent) messages

    pthread_cond_t* senderEmptyQueueCond;// signaling sender thread from main thread
    pthread_mutex_t* senderEmptyQueueMutex;// signaling sender thread from main thread

    pthread_cond_t* rec2SenderCond; // signaling sender thread from receiver thread
    pthread_mutex_t* rec2SenderMutex; // signaling sender thread from receiver thread

    pthread_cond_t* mainCond; // signaling sender thread from receiver thread
    pthread_mutex_t* mainMutex; // signaling sender thread from receiver thread
} ThreadCommunication;

typedef struct ProgramInterface {
    CommunicationDetails* comDetails;
    struct NetworkConfig* netConfig;
    ThreadCommunication* threads;
} ProgramInterface;

// ----------------------------------------------------------------------------
// Functions
// ----------------------------------------------------------------------------

/**
 * @brief Prints help menu when user inputs /help command 
 */
void printCliHelpMenu(const char* executableName);

/**
 * @brief Prints user help menu in running client
 * 
 * @param progInt Pointer to ProgramInterface for thread-safe printing
 */
void printUserHelpMenu(ProgramInterface* progInt);

/**
 * @brief Changes program state to new state with thread protecion using mutex
 * 
 * @param newState New state to be set
 */
void setProgramState(ProgramInterface* progInt, fsm_t newState);

/**
 * @brief Returns program state with thread protecion using mutex
 */
fsm_t getProgramState(ProgramInterface* progInt);


#endif /*PROGRAM_INTERFACE_H*/