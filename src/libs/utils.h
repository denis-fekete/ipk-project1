/**
 * @file utils.h
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef UTILS_H
#define UTILS_H 1

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "stdbool.h"
#include "pthread.h"
#include "unistd.h"

#include "buffer.h" 

// ----------------------------------------------------------------------------
//  Enums
// ----------------------------------------------------------------------------

typedef enum FSM {
    fsm_START, 
    fsm_AUTH_W82_BE_SENDED, /*authentication is waiting(W8) to(2) be sended*/
    fsm_AUTH_SENDED, /*authetication was successfully sended*/
    fsm_W8_4_REPLY, /*auth has been confirmed, waiting for reply*/
    fsm_OPEN, /*authetication was confirmed and replied to*/
    fsm_ERR, 
    fsm_BYE,
    fms_END} fsm_t;

typedef enum CommandType {cmd_AUTH, cmd_JOIN, cmd_RENAME, cmd_HELP, cmd_CONF, cmd_MSG, cmd_EXIT, cmd_NONE} cmd_t;

typedef enum Protocols {prot_ERR=-50, prot_UDP=50, prot_TCP=100} prot_t;

typedef enum MessageType {
    msg_CONF = 0x00,
    msg_REPLY = 0x01,
    msg_AUTH = 0x02,
    msg_JOIN = 0x03,
    msg_MSG = 0x04,
    msg_ERR = 0xFE,
    msg_BYE = 0xFF
  } msg_t;

// ----------------------------------------------------------------------------
//  Structures
// ----------------------------------------------------------------------------

/**
 * @brief Points at the start of the bytes block (array) with len length.
 * 
 * @note Byte block in this context does not own the memory, it is just a 
 * pointer with length
 * 
 */
typedef struct BytesBlock {
    char* start; // pointer to the starting character of the block
    size_t len; // length of the block
} BytesBlock;

#include "customtypes.h"

typedef struct CommunicationDetails {
    Buffer displayName;
    Buffer channelID;
    u_int16_t msgCounter;
} CommunicationDetails;


typedef struct ThreadCommunication {
    // true = work as normal, false = prepare to end
    bool continueProgram; // variable to signal that program should prepare for finishing
    fsm_t fsmState; // state of FSM, how should program behave 

    pthread_mutex_t* stdoutMutex;
    pthread_mutex_t* fsmMutex;

    struct MessageQueue* sendingQueue; // queue of outcoming (user sent) messages
    struct MessageQueue* receivedQueue; // queue of incoming (server sent) messages

    pthread_cond_t* senderEmptyQueueCond;// signaling sender thread from main thread
    pthread_mutex_t* senderEmptyQueueMutex;// signaling sender thread from main thread

    pthread_cond_t* rec2SenderCond; // signaling sender thread from receiver thread
    pthread_mutex_t* rec2SenderMutex; // signaling sender thread from receiver thread
} ThreadCommunication;

typedef struct ProgramInterface {
    CommunicationDetails* comDetails;
    struct NetworkConfig* netConfig;
    ThreadCommunication* threads;
} ProgramInterface;
// ----------------------------------------------------------------------------
//  Functions
// ----------------------------------------------------------------------------

/**
 * @brief Print error message to stderr and exit program with errorCode
 * 
 * @param msg Message to be printed
 * @param errorCode Error code to be exited with
 */
int errHandling(const char* msg, int errorCode);

/**
 * @brief Prints message to the user, used for feedback no error messages
 * 
 * @param msg Message to be printed
 */
void displayMsgToUser(const char* msg);

/**
 * @brief Fills BytesBlock variable from last word found 
 * Skips all blank characters (spaces and tabs) 
 * @param block Variable of initialized 
 * @param startOfLastWord 
 * @param bufferSize 
 */
void getWord(BytesBlock* block, char* startOfLastWord, size_t bufferSize);
/**
 * @brief Finds first blank character (spaces ' ' and tabulators '\t') 
 * in string and returns index of last character before blank character
 * 
 * @param string Input string to be searched in
 * @param len Length of string that cannot be exceeded
 * @return long Index in string
 */
long findBlankCharInString(char* string, size_t len);

/**
 * @brief Skips blank character until a first non empty character is found
 * 
 * @param string String to be searched in
 * @param len Length of string that cannot be 
 * @return long 
 */
long skipBlankCharsInString(char* string, size_t len);

/**
 * @brief Returns index of last character before '\0' character in string
 * 
 * @param string Pointer to the string
 * @param len Length of the string
 * @return long Index of last character before \0
 */
long findZeroInString(char* string, size_t len);

/**
 * @brief Returns true if character is considered as an ending character for
 * command
 * 
 * @param input Input character
 * @return int If yes 1 (True), 0 (False)
 */
int isEndingCharacter(char input);

/**
 * @brief Replaces bytes in dst with bytes from src up to len lenght
 * 
 * @param dst Destinatin byte array
 * @param src Source byte arry
 * @param len Number of bytes to replace
 */
void stringReplace(char* dst, char* src, size_t len);

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


/**
 * @brief Adds ONE to message counter
 * 
 * @param newState New state to be set
 */
void incMessageCounter(ProgramInterface* progInt);

/**
 * @brief Returns message counter
 */
uint16_t getMessageCounter(ProgramInterface* progInt);

/**
 * @brief Macro for safe printing using "global" stdoutMutex.
 * 
 * Uses fflush after message has been written
 */
#define safePrintStdout(...) \
    pthread_mutex_lock(progInt->threads->stdoutMutex);  \
    printf(__VA_ARGS__);                                \
    fflush(stdout);                                     \
    pthread_mutex_unlock(progInt->threads->stdoutMutex);\

#ifdef DEBUG
    extern pthread_mutex_t debugPrintMutex;

    #define debugPrint(...)                     \
        pthread_mutex_lock(&debugPrintMutex);   \
        fprintf(__VA_ARGS__);                   \
        pthread_mutex_unlock(&debugPrintMutex);
        
    #define debugPrintSeparator(fs)                                 \
        pthread_mutex_lock(&debugPrintMutex);                       \
        fprintf(fs, "----------------------------------------\n");  \
        pthread_mutex_unlock(&debugPrintMutex);
#else
    #define debugPrint(...) ;
    #define debugPrintSeparator(fs) ;
#endif

#endif /*UTILS_H*/