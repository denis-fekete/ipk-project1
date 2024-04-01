/**
 * @file serverTCP.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief Test server for TCP communication (temporary)
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "arpa/inet.h"
#include "netdb.h"
#include "sys/socket.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

enum errExits {BAD_HOSTNAME = 2, SENDING_FAILED, SOCKET_CREATION_FAIL, BIND_ERROR, RECEIVING_FAILED};

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
        if(string[index] == '\0' || string[index] == '\n' || string[index] == '\r')
        { 
            return index;
        }
    }

    // Character was not found
    return -1;
}

int findStrInStr(const char* strToBeFound, char* findIn, int len)
{
    int strToBeFoundLen = strlen(strToBeFound);
    for(int i = 0; i <= len - strToBeFoundLen; i++)
    {
        int k;
        for(k = 0; k < strToBeFoundLen; k++)
        {
            if(findIn[i + k] != strToBeFound[k]) {
                break; // Break the inner loop if characters don't match
            }
        }
        if(k == strToBeFoundLen) { 
            return i; // Return the starting index of the found string
        }
    }
    return -1; // Return -1 if string is not found
}

#define print_buffer(length) \
    for(int i = 0; (i < BUFFER_SIZE) && (i < length); i++)\
    {\
        if((buffer[i] >= 'a' && buffer[i] <= 'z') || (buffer[i] >= 'A' && buffer[i] <= 'Z') || \
            buffer[i] >= '0' && buffer [i] <= '9') \
        { \
            printf( "%c" , (unsigned char) buffer[i]);\
        } \
        else \
        {\
            printf( "(%hhx)" , (unsigned char) buffer[i]);\
        }\
    }\
    printf("\n");

int main()
{
    typedef enum ServerMode {DO_NOTHING, RESEND_ALL, CONFIRM_ALL, REPLY_AND_CONFIRM_ALL} ServerMode;
    int serverMode = REPLY_AND_CONFIRM_ALL;

    // ----------------------------------------------------
    // Creating socket
    // ----------------------------------------------------
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket <= 0)
    {
        fprintf(stderr, "ERROR: socket\n");
        // exit(SOCKET_CREATION_FAIL);
    }

    int enable = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    // ----------------------------------------------------
    // Binding
    // ----------------------------------------------------
    unsigned short serverPort = 4567;
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(4567);

    struct sockaddr* address = (struct sockaddr*) &serverAddress;
    int addressSize = sizeof(serverAddress);

    if(bind(serverSocket, address, addressSize) < 0)
    {
        fprintf(stderr, "ERROR: Failed to bind\n");
        exit(BIND_ERROR);
    }

    // ----------------------------------------------------
    // Data receiving and sending response
    // ----------------------------------------------------
    struct sockaddr_in clientAddress;
    socklen_t clientAddressSize = sizeof(clientAddress);
    struct sockaddr* addressC = (struct sockaddr*) &clientAddress;

    #define BUFFER_SIZE 1500
    char buffer[BUFFER_SIZE] = {0};
    char bufferSend[BUFFER_SIZE] = {0};

    int max_waiting_connections = 1;
    if (listen(serverSocket, max_waiting_connections) < 0)
    {
        perror("ERROR: listen");
        exit(EXIT_FAILURE);
    }

    struct sockaddr comm_addr;
    socklen_t comm_addr_size;
    int ignoreClient = 0;
    int comm_socket = accept(serverSocket, &comm_addr, &comm_addr_size);
    int loop = 1;

    // ----------------------------------------------------
    // Main loop
    // ----------------------------------------------------
    while(loop)
    {
        for(int i = 0; i < BUFFER_SIZE; i++)
        { buffer[i] = '\0'; }

        int bytes_rx = recvfrom(comm_socket, buffer, BUFFER_SIZE, 0, &comm_addr, &comm_addr_size);

        if(bytes_rx < 0)
        {
            fprintf(stderr, "ERROR: recvfrom\n");
            exit(RECEIVING_FAILED);
        }
        int bytes_tx;
        
        printf("Bytes recieved (%d):", bytes_rx);
        printf("%s", buffer);

        int index;
        char* username = NULL;
        int username_len = 0;
        char* displayname = NULL;
        int displayname_len = 0;
        char* msgcontents = NULL;
        int msgcontents_len = 0;


        // if message is auth
        if(strncmp(buffer, "AUTH", 4) == 0 || strncmp(buffer, "JOIN", 4) == 0)
        {
            if(ignoreClient) {continue;}
            // get username 
            username = &(buffer[sizeof("AUTH ") - 1]);
            username_len = findBlankCharInString(username, BUFFER_SIZE);
            // check for possible options of server
            if(strncmp(username, "sendMeErr", sizeof("sendMeRrr") - 1) == 0)
            {
                sprintf(bufferSend, "ERR FROM SERVER IS Unknown error\r\n");
            }
            else if(strncmp(username, "sendBadMsg", sizeof("sendBadMsg") - 1) == 0)
            {
                sprintf(bufferSend, "SOME RANDOM CHARACTERS\r\n");
            }
            else if(strncmp(username, "sendBye", sizeof("sendBye") - 1) == 0)
            {
                sprintf(bufferSend, "BYE\r\n");
            }
            else if(strncmp(username, "ignoreMe", sizeof("ignoreMe") - 1) == 0)
            {
                ignoreClient = 1;
                continue;
            }
            else // else send reply
            {
                index = findStrInStr(" AS ", &(username[username_len]), BUFFER_SIZE - username_len);
                displayname = &(username[username_len + index]);
                displayname_len = findBlankCharInString(displayname, BUFFER_SIZE);
                // if username starts with a reply is OK
                if(username[0] == 'a')
                {
                    sprintf(bufferSend, "REPLY OK IS OKE\r\n");
                }
                else
                {
                    sprintf(bufferSend, "REPLY NOK IS NOKE\r\n");
                }
            }
 
            // send response
            int len = strlen(bufferSend);
            bytes_tx = sendto(comm_socket, bufferSend, len, 0, &comm_addr, comm_addr_size);
            printf("Bytes send (%d):", bytes_tx);
            printf("%s", buffer);
        }
        else if(strncmp(buffer, "MSG", 3) == 0)
        {
            // if message was received send back same message but with SEVER as
            // sender
            if(ignoreClient) {continue;}

            displayname = &(buffer[sizeof("MSG FROM ") - 1]);
            displayname_len = findBlankCharInString(displayname, BUFFER_SIZE);

            index = findStrInStr(" IS ", buffer, BUFFER_SIZE);
            printf("found index %i\n", index);
            msgcontents = &(buffer[index + sizeof(" IS ") - 1]);
            msgcontents_len = findNewLineInString(msgcontents, BUFFER_SIZE);

            sprintf(bufferSend, "MSG FROM SERVER IS ");
            int currStrLen = strlen(bufferSend);

            for(int i = 0; i < msgcontents_len; i++)
            {
                bufferSend[currStrLen + i] = msgcontents[i];
            }
            // correctly end message
            bufferSend[currStrLen + msgcontents_len ] = '\r';
            bufferSend[currStrLen + msgcontents_len + 1] = '\n';
            bufferSend[currStrLen + msgcontents_len + 2] = '\0';
            // send message to client
            int len = strlen(bufferSend);
            bytes_tx = sendto(comm_socket, bufferSend, len, 0, &comm_addr, comm_addr_size);
            printf("Bytes send (%d):", bytes_tx);
            printf("%s", bufferSend);
        }
        else if(strncmp(buffer, "BYE", 3) == 0)
        {
            loop = 0;
            if(ignoreClient) 
            {
                ignoreClient = 0;
                continue;
            }

        }
        else
        {
            print_buffer(bytes_rx);
        }
        

        if(bytes_tx < 0)
        {
            fprintf(stderr, "ERROR: sendto\n");
            exit(SENDING_FAILED);
        }
    }

    shutdown(comm_socket, SHUT_RDWR);
    close(comm_socket);
    shutdown(serverSocket, SHUT_RDWR);
}