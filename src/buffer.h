#ifndef BUFFER_H
#define BUFFER_H 1

#include "stdlib.h"
#include "stdio.h"

#include "utils.h"
#include "customtypes.h"


#define INITIAL_BUFFER_SIZE 256

void bufferReset(Buffer* buffer);

void bufferResize(Buffer* buffer, size_t newSize);

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
// size_t loadBuffer(char** buffer, size_t* bufferSize)
size_t loadBufferFromStdin(Buffer* buffer);

/**
 * @brief Prints buffer characters byte by byte from start to used
 * 
 * @param buffer Input buffer
 * @param hex If not 0 (false) prints hex values with white spaces between
 * @param smartfilter If not 0 (false) only alphanumeric chacters will be prited
 * as chracaters and other chars will be printed as hex codes
 */
void printBuffer(Buffer* buffer, int hex, int smartfilter);

#endif