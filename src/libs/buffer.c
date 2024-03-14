/**
 * @file buffer.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "buffer.h"

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

void bufferClear(Buffer* buffer)
{
    buffer->data = NULL;
    buffer->allocated = 0;
    buffer->used = 0;
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

void bufferResize(Buffer* buffer, size_t newSize)
{
    if(newSize <= buffer->allocated)
    {
        return;
    }

    // Realloc buffer
    char* tmp = realloc(buffer->data, newSize);
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

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

void printBuffer(Buffer* buffer, int hex, int smallfilter)
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