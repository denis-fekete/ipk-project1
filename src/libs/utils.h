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

#include "buffer.h" 
#include "customtypes.h"

typedef enum CommandType {AUTH, JOIN, RENAME, HELP, PLAIN_MSG, CMD_EXIT, NONE} cmd_t;
enum Protocols {ERR, UDP, TCP};
typedef enum MessageType {
    MSG_CONF = 0x00,
    MSG_REPLY = 0x01,
    MSG_AUTH = 0x02,
    MSG_JOIN = 0x03,
    MSG_MSG = 0x04,
    MSG_ERR = 0xFE,
    MSG_BYE = 0xFF
  } msg_t;

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
 * @brief Prints ByteBlock to stdout
 * 
 * @param block Block to be printed
 * @param hex If not 0 (false) block will be printed in hexadecimal values
 */
void printByteBlock(BytesBlock* block, int hex);

/**
 * @brief Replaces bytes in dst with bytes from src up to len lenght
 * 
 * @param dst Destinatin byte array
 * @param src Source byte arry
 * @param len Number of bytes to replace
 */
void stringReplace(char* dst, char* src, size_t len);



#endif