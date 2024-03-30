/**
 * @file utils.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "utils.h"

/**
 * @brief Prints msg and exits program with errorCode
 * 
 * @param msg Message to be printed
 * @param errorCode Error code that will be used as exit code
 * @return int Returns 0 (for anti-compiler errors) 
 */
int errHandling(const char* msg, int errorCode)
{
    fprintf(stderr, "ERR: %s\n", msg);
    exit(errorCode);
    return 0;
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

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
bool getWord(BytesBlock* block, char* startOfLastWord, size_t bufferSize)
{
    if(block == NULL || startOfLastWord == NULL || bufferSize <= 0)
    {
        errHandling("Invalid input variables in fuction getWord()", 1); // TODO: change err code
    }

    size_t index = skipBlankCharsInString(startOfLastWord, bufferSize);
    block->start = &(startOfLastWord[index]);

    index = findBlankCharInString(block->start, bufferSize - block->len);
    block->len = index;
    
    // if block lenght is not zero returns true ass success, else failed
    return (block->len != 0) ? true : false;
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------


/**
 * @brief Finds first blank character (spaces ' ' and tabulators '\t') 
 * in string and returns index of last character before blank character
 * 
 * @param string Input string to be searched in
 * @param len Length of string that cannot be exceeded
 * @return long Index in string
 */
long findBlankCharInString(char* string, size_t len)
{
    size_t index = 0;
    for(; index <= len ; index++)
    {
        if( string[index] == ' ' || string[index] == '\t' || 
            string[index] == '\0' || string[index] == '\n' ||
            string[index] == '\r')
        { 
            return index;
        }
    }

    // Character was not found
    return -1;
}

/**
 * @brief Finds new line character in string
 * 
 * @param string Input string
 * @param len Maximum length
 * @return long Index in newline was found
 */
long findNewLineInString(char* string, size_t len)
{
    size_t index = 0;
    for(; index <= len ; index++)
    {
        if(string[index] == '\0' || string[index] == '\n')
        { 
            return index;
        }
    }

    // Character was not found
    return -1;
}

/**
 * @brief Skips blank character until a first non empty character is found
 * 
 * @param string String to be searched in
 * @param len Length of string that cannot be 
 * @return long 
 */
long skipBlankCharsInString(char* string, size_t len)
{
    size_t index = 0;
    for(; index < len ; index++)
    {
        if(! (string[index] == ' ' || string[index] == '\t'))
        { 
            return index;
        }

        if(string[index] == '\0' || string[index] == '\n')
        {
            // Character was not found
            return -1;
        }
    }

    // Character was not found
    return -1;
}

/**
 * @brief Returns index of last character before '\0' character in string
 * 
 * @param string Pointer to the string
 * @param len Length of the string
 * @return long Index of last character before \0
 */
long findZeroInString(char* string, size_t len)
{
    long index = 0;
    while( ((size_t)index < len) && (string[index] != 0) )
    {
        index++;
    }

    if(index <= 0)
    {
        return errHandling("Intrnal error in findZeroInString()", 1); // TODO: change error code
    }
    else { return index; }
}

/**
 * @brief Returns true if character is considered as an ending character for
 * command
 * 
 * @param input Input character
 * @return int If yes 1 (True), 0 (False)
 */
int isEndingCharacter(char input)
{
    // 3 = Ctrl-D, 4 = Ctrl-C
    if(input == EOF || input == '\n' || input == 3 || input == 4)
    {
        return 0;
    }

    return 1;
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Replaces bytes in dst with bytes from src up to len lenght
 * 
 * @param dst Destinatin byte array
 * @param src Source byte arry
 * @param len Number of bytes to replace
 */
void stringReplace(char* dst, char* src, size_t len)
{
    if(dst != NULL && src != NULL)
    {
        for(size_t i = 0; i < len; i++)
        {
            dst[i] = src[i];
        }
        return;
    }

    #ifdef DEBUG
        fprintf(stderr, "Error: stringReplace() received bad pointer or "
        "invalid length (dst:%p, src:%p, len:%ld\n", dst, src, len);
    #endif
}


// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Converts unsigned char into an CommandType
 * 
 * @param input Input unsigned char
 * @return cmd_t Returned value in cmd_t 
 */
cmd_t uchar2CommandType(unsigned char input)
{
    int tmp = input;
    if(tmp >= cmd_AUTH && tmp <= cmd_NONE)
    {
        return (enum CommandType) tmp;
    }

    return cmd_CONVERSION_ERR;
}

/**
 * @brief Converts 16bit message id into an two unsinged chars
 * 
 * @param high Output pointer to unsigned char, upper/higher half of number
 * @param low Output pointer to unsigned char, lower half of number
 * @param msgCounter Input number to be separated
 */
void breakU16IntToBytes(char* high, char* low, uint16_t msgCounter)
{
    *high = (unsigned char)((msgCounter) >> 8);
    *low = (unsigned char)((msgCounter) & 0xff);
}

/**
 * @brief Converts bwo bytes from input char array into 16bit usigned integer
 * 
 * @param high Higher byte
 * @param low Lower byte
 * @return uint16_t 
 */
uint16_t convert2BytesToU16Int(char low, char high)
{
    // Join bytes into one number
    return (low +  (high << 8)); 
}