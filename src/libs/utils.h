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
#include "stdint.h"
#include "unistd.h"

// ----------------------------------------------------------------------------
//  Enums
// ----------------------------------------------------------------------------

typedef enum FSM {
    fsm_START, 
    fsm_AUTH_W82_BE_SENDED, /*authentication is waiting(W8) to(2) be sended*/
    fsm_AUTH_SENDED, /*authetication was successfully sended*/
    fsm_W8_4_REPLY, /*auth has been confirmed, waiting for reply*/
    fsm_W8_4_CONF, /*reply received, waiting for confirm to be sended*/
    fsm_OPEN, /*authetication was confirmed and replied to*/
    fsm_JOIN_ATEMPT, /*attempted to join another channale, wait for response*/
    fsm_EMPTY_Q_BYE, /*send all messages left in queue and then end*/
    fsm_BYE_RECV, /*bye was received, prepare to end program*/
    fsm_ERR, 
    fsm_END
    } fsm_t;

typedef enum CommandType {
    cmd_AUTH, 
    cmd_JOIN, 
    cmd_RENAME, 
    cmd_HELP, 
    cmd_CONF, 
    cmd_MSG, 
    cmd_EXIT, 
    cmd_NONE,
    cmd_CONVERSION_ERR
    } cmd_t;



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
 * @brief Returns word from string. Look from first character until 
 * blank character is found
 * 
 * @param block BytesBlock to which should the result be saved
 * @param startOfLastWord Pointer to the start of the last word
 * @param bufferSize Maximum size of buffer
 * @return true Success
 * @return false Failed
 */
bool getWord(BytesBlock* block, char* startOfLastWord, size_t bufferSize);
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
 * @brief Finds new line character in string
 * 
 * @param string Input string
 * @param len Maximum length
 * @return long Index in newline was found
 */
long findNewLineInString(char* string, size_t len);

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
 * @brief Converts unsigned char into an CommandType
 * 
 * @param input Input unsigned char
 * @return cmd_t Returned value in cmd_t 
 */
cmd_t uchar2CommandType(unsigned char input);

/**
 * @brief Converts 16bit message id into an two unsinged chars
 * 
 * @param high Output pointer to unsigned char, upper/higher half of number
 * @param low Output pointer to unsigned char, lower half of number
 * @param msgCounter Input number to be separated
 */
void breakU16IntToBytes(char* high, char* low, uint16_t msgCounter);

/**
 * @brief Converts bwo bytes from input char array into 16bit usigned integer
 * 
 * @param high Higher byte
 * @param low Lower byte
 * @return uint16_t 
 */
uint16_t convert2BytesToU16Int(char high, char low);

/**
 * @brief Macro for safe printing using "global" stdoutMutex.
 * 
 * Uses fflush after message has been written
 */
#define safePrintStdout(...) \
    pthread_mutex_lock(progInt->threads->stdoutMutex);  \
    printf(__VA_ARGS__);                                \
    fflush(stdout);                                     \
    pthread_mutex_unlock(progInt->threads->stdoutMutex);


#define safePrintStderr(...) \
    pthread_mutex_lock(progInt->threads->stdoutMutex);  \
    fprintf(stderr, __VA_ARGS__);                      \
    fflush(stderr);                                     \
    pthread_mutex_unlock(progInt->threads->stdoutMutex);

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