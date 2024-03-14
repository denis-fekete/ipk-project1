/**
 * @file buffer.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "buffer.h"

/**
 * @brief Sets default values to the buffer
 * 
 * @warning Do not use on buffer that already has allocated memory
 * 
 * @param buffer Buffer to be reseted
 */
void bufferInit(Buffer* buffer)
{
    buffer->data = NULL;
    buffer->allocated = 0;
    buffer->used = 0;
}

/**
 * @brief Resizes buffer to new size, if buffer is not
 * iniatilized (NULL) default value (INITIAL_BUFFER_SIZE) will
 * be used instead to prevent allocation of small buffers
 * 
 * @param buffer Buffer to be resized
 * @param newSize New size
 */
void bufferResize(Buffer* buffer, size_t newSize)
{
    if(newSize <= buffer->allocated)
    {
        return;
    }

    // Realloc buffer
    char* tmp = (char*) realloc(buffer->data, newSize);
    // Check for failed memory reallocation
    if(tmp == NULL)
    {
        fprintf(stderr, "ERROR: Realloc failed");
        exit(1); // TODO: error code change
    }
    // Save new value to buffer and bufferSize
    buffer->allocated = newSize;
    buffer->data = tmp;
}

/**
 * @brief Copies contents from src buffer to dst
 * 
 * @param dst Buffer to which data will be copied
 * @param src Buffer from which data will be copied
 */
void bufferCopy(Buffer* dst, Buffer* src)
{
    if(dst == NULL || src == NULL)
    {
        errHandling("In bufferCopy src or dst pointers are null", 1); // TODO:
    }

    // If dst is smaller than src, resize it
    if(dst->allocated < src->used)
    {
        bufferResize(dst, src->used);
    }

    for (size_t i = 0; i < src->used; i++)
    {
        dst->data[i] = src->data[i];
    }

    dst->used = src->used;
}

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
size_t loadBufferFromStdin(Buffer* buffer)
{
    char c = getc(stdin);
    
    size_t i = 0;
    for(; isEndingCharacter(c) ; i++)
    {
        
        // resize buffer if smaller than num. of loaded characters or zero
        if(buffer->data == NULL || buffer->allocated <= 0) 
        {
            bufferResize(buffer, INITIAL_BUFFER_SIZE);
        }
        else if((buffer->allocated - 1) < i)
        {
            bufferResize(buffer, buffer->allocated * 2);
        }

        buffer->data[i] = c;
        c = getc(stdin);
    }

    // Add 0 to the end of string
    buffer->data[i] = '\0';

    return i;
}

/**
 * @brief Prints buffer characters byte by byte from start to used
 * 
 * @param buffer Input buffer
 * @param hex If not 0 (false) prints hex values with white spaces between
 * @param smartfilter If not 0 (false) only alphanumeric chacters will be prited
 * as chracaters and other chars will be printed as hex codes
 */
void bufferPrint(Buffer* buffer, int hex, int smallfilter)
{
    if(buffer->data == NULL || buffer->used == 0) { return; }

    for(size_t i = 0; i < buffer->used; i++)
    {
        if(hex) { printf("%hhx ", (unsigned char)buffer->data[i]); }
        else
        {
            if(smallfilter)
            {
                char c = buffer->data[i];
                if( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
                {
                    printf("%c", c);
                }
                else 
                {
                    printf("(%hhx)", (unsigned char)c);
                }
            }
            else { printf("%c", buffer->data[i]); }
        }
    }

    printf("\n");
}

/**
 * @brief Destroys Buffer and frees memory
 * 
 * @param buffer 
 */
void bufferDestory(Buffer* buffer)
{
    printf("Destroying pointer %p\n", (void*)buffer->data);
    free(buffer->data);
}