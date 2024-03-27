/**
 * @file buffer.h
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef BUFFER_H
#define BUFFER_H 1

#include "stdlib.h"
#include "stdio.h"

#include "utils.h"

#define INITIAL_BUFFER_SIZE 256

/**
 * @brief Buffer is an structure for defining byte arrays (char arrays / string)
 * with information about how many bytes has been allocated and how many 
 * has been used.
 */
typedef struct Buffer
{
    char* data;
    size_t allocated ;
    size_t used;
} Buffer;


/**
 * @brief Sets default values to the buffer
 * 
 * @warning Do not use on buffer that already has allocated memory
 * 
 * @param buffer Buffer to be reseted
 */
void bufferInit(Buffer* buffer);

/**
 * @brief Resizes buffer to new size, if buffer is not
 * iniatilized (NULL) default value (INITIAL_BUFFER_SIZE) will
 * be used instead to prevent allocation of small buffers
 * 
 * @param buffer Buffer to be resized
 * @param newSize New size
 */
void bufferResize(Buffer* buffer, size_t newSize);

/**
 * @brief Copies contents from src buffer to dst
 * 
 * @param dst Buffer to which data will be copied
 * @param src Buffer from which data will be copied
 */
void bufferCopy(Buffer* dst, Buffer* src);

/**
 * @brief Fills buffer with characters from stdin.
 * 
 * Fills buffer character by character until EOF is found.
 * If buffer is running out of space, it will be resized
 * 
 * @param buffer Pointer to the buffer. Can be inputed as NULL, however correct buffer size
 * is required
 * @param bufferSize Pointer size of provided buffer
 */
size_t loadBufferFromStdin(Buffer* buffer, bool* eofDetected);

/**
 * @brief Prints buffer characters byte by byte from start to used
 * 
 * @param buffer Input buffer
 * @param hex If not 0 (false) prints hex values with white spaces between
 * @param smartfilter If not 0 (false) only alphanumeric chacters will be prited
 * as chracaters and other chars will be printed as hex codes
 */
void bufferPrint(Buffer* buffer, int useDebugPrint);

/**
 * @brief Destroys Buffer and frees memory
 * 
 * @param buffer 
 */
void bufferDestory(Buffer* buffer);

#endif