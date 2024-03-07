#ifndef UTILS_H
#define UTILS_H 1

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "buffer.h" 
#include "customtypes.h"

typedef enum CommandType {AUTH, JOIN, RENAME, HELP, PLAIN_MSG, CMD_EXIT} cmd_t;
enum Protocols {UDP, TCP};

#define MSG_TYPE_CONFIRM 0x00
#define MSG_TYPE_REPLY 0x01
#define MSG_TYPE_AUTH 0x02
#define MSG_TYPE_JOIN 0x03
#define MSG_TYPE_MSG 0x04
#define MSG_TYPE_ERR 0xFE
#define MSG_TYPE_BYE 0xFF


int errHandling(const char* msg, int errorCode);

/**
 * @brief Fills BytesBlock variable from last word found 
 * Skips all blank characters (spaces and tabs) 
 * @param block Variable of initialized 
 * @param startOfLastWord 
 * @param bufferSize 
 */
void getWord(BytesBlock* block, char* startOfLastWord, size_t bufferSize);
/**
 * @brief Finds first blank character (spaces ' ' and tabulators '\t') in string and returns index 
 * of last character before blank character
 * 
 * @param string Input string to be searched in
 * @param len Length of string
 * @return long Index in string
 */
long findBlankCharInString(char* string, size_t len);

/**
 * @brief 
 * 
 * @param string 
 * @param len 
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