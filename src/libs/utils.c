/**
 * @file utils.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "utils.h"

int errHandling(const char* msg, int errorCode)
{
    fprintf(stderr, "ERROR: %s\n", msg);
    exit(errorCode);
    return 0;
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

long findZeroInString(char* string, size_t len)
{
    long index = 0;
    for(; ((size_t)index < len) && (string[index] != '\0') ; index++) {}

    // -1 to return index of last character before \0
    index = index - 1;

    if(index < 0)
    {
        if(index == -1) { return 0; }
        else
        {
            return errHandling("Intrnal error in findZeroInString()", 1); // TODO: change error code
        }
    }
    else { return index; }
}


// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

void getWord(BytesBlock* block, char* startOfLastWord, size_t bufferSize)
{
    if(block == NULL || startOfLastWord == NULL || bufferSize <= 0)
    {
        errHandling("Invalid input variables in fuction getWord()", 1); // TODO: change err code
    }

    size_t index = skipBlankCharsInString(startOfLastWord, bufferSize);
    // +1 because 
    block->start = &(startOfLastWord[index]);

    index = findBlankCharInString(block->start, bufferSize - block->len);
    block->len = index;
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

long findBlankCharInString(char* string, size_t len)
{
    size_t index = 0;
    for(; index <= len ; index++)
    {
        if(string[index] == ' ' || string[index] == '\t' || string[index] == '\0' || string[index] == '\n')
        { 
            return index;
        }
    }

    // Character was not found
    return -1;
}



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

void printByteBlock(BytesBlock* block, int hex)
{
    if(block->start == NULL || block->len == 0) { return; }

    for(size_t i = 0; i < block->len; i++)
    {
        if(hex)
        {
            printf("%x ", (block->start)[i]);
        }
        else
        {
            printf("%c", (block->start)[i]);
        }
    }

    printf("\n");
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

void stringReplace(char* dst, char* src, size_t len)
{
    if(dst == NULL || src == NULL || len == 0)
    {
        #ifdef DEBUG
        fprintf(stderr, "Error: stringReplace() received bad pointer or invalid length (dst:%p, src:%p, len:%ld\n", dst, src, len);
        #endif
    }

    for(size_t i = 0; i < len; i++)
    {
        dst[i] = src[i];
    }
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------


void displayMsgToUser(const char* msg)
{
    printf("Application: %s\n", msg);
}

