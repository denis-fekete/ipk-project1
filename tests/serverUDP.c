/**
 * @file serverUDP.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief Test server for UPD communication (temporary)
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
    int serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if(serverSocket <= 0)
    {
        fprintf(stderr, "ERROR: socket\n");
        exit(SOCKET_CREATION_FAIL);
    }

    // ----------------------------------------------------
    // Binding
    // ----------------------------------------------------
    unsigned short serverPort = 0;
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(4567);

    struct sockaddr* address = (struct sockaddr*) &serverAddress;
    int addressSize = sizeof(serverAddress);
    int ignoreClient = 0;
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

    #define BUFFER_SIZE 512
    char buffer[BUFFER_SIZE] = {0};

    int once = 1;
    while(once)
    {
        for(int i = 0; i < BUFFER_SIZE; i++)
        { buffer[i] = '\0'; }

        int bytes_rx = recvfrom(serverSocket, buffer, BUFFER_SIZE, 0, addressC, &clientAddressSize);

        if(bytes_rx < 0)
        {
            fprintf(stderr, "ERROR: recvfrom\n");
            exit(RECEIVING_FAILED);
        }

        int bytes_tx;
        
        
        printf("Bytes recieved: %d\n", bytes_rx);
        print_buffer(bytes_rx);

        if(buffer[0] == 0x02 || buffer[0] == 0x03) // AUTH received
        {
            if(ignoreClient) {continue;}
            int msgLen;
            //// check for possible options of server
            if(strncmp(&buffer[3], "sendMeErr", sizeof("sendMeErr") - 1) == 0)
            {
                buffer[0] = 0xFE; // set ass err
                // leave buffer[2] as is

                buffer[3] = 'S';
                buffer[4] = 'E';
                buffer[5] = 'R';
                buffer[6] = 'V';
                buffer[7] = 'E';
                buffer[8] = 'R';
                buffer[9] = 0;
                buffer[10] = 'E';
                buffer[11] = 'R';
                buffer[12] = 'R';
                buffer[13] = 'O';
                buffer[14] = 'R';
                buffer[15] = 0;
                msgLen = 16;
            }
            else if(strncmp(&buffer[3], "sendBadMsg", sizeof("sendBadMsg") - 1) == 0)
            {
                buffer[0] = 100;
                buffer[1] = 90;
                buffer[2] = 80;
                buffer[3] = 70;
                msgLen = 4;
            }
            else if(strncmp(&buffer[3], "sendMeBye", sizeof("sendMeBye") - 1) == 0)
            {
                // send confirm
                buffer[0] = 0x00;
                buffer[3] = 0;
                bytes_tx = sendto(serverSocket, buffer, 3, 0, addressC, clientAddressSize);
                printf("Bytes sended (%i):\n", bytes_tx);
                print_buffer(bytes_tx);

                buffer[0] = 0xff;
                buffer[1] += 1;
                // buffer[2];
                msgLen = 3;
            }
            else if(strncmp(&buffer[3], "ignoreMe", sizeof("ignoreMe") - 1) == 0)
            {
                printf("ignoring client\n");
                ignoreClient = 1;
                continue;;
            }
            else
            {
                int reply;
                if(buffer[3] == 'a')
                { reply = 1;}
                else
                { reply = 0; }

                // send confirm
                buffer[0] = 0x00;
                buffer[3] = 0;
                bytes_tx = sendto(serverSocket, buffer, 3, 0, addressC, clientAddressSize);
                printf("Bytes sended (%i):\n", bytes_tx);
                print_buffer(bytes_tx);

                buffer[0] = 0x01; // SET AS REPLY
                buffer[4] = buffer[1]; // copy sended msg id as ref msg id
                buffer[5] = buffer[2]; // copy sended msg id as ref msg id
                if(reply)
                {
                    buffer[3] = 0x01; // set result as success
                    buffer[6] = 'O';
                    buffer[7] = 'K';
                    buffer[8] = 'E';
                }
                else
                {
                    buffer[3] = 0x00; // set result as success
                    buffer[6] = 'B';
                    buffer[7] = 'A';
                    buffer[8] = 'D';
                }
                buffer[9] = '\0';
                msgLen = 10;
            }

            // increase counter
            buffer[1] = ((unsigned char) buffer[1]) + 1;
            bytes_tx = sendto(serverSocket, buffer, msgLen, 0, addressC, clientAddressSize);
            
            printf("Bytes sended (%i):\n", bytes_tx);
            print_buffer(bytes_tx);
        }
        else if(buffer[0] == 0x04) // received MSG
        {
            if(ignoreClient) {continue;}
            // find first 0 byte, start at 2 to skip MessageID
            int i = 2;
            for(; i < bytes_rx; i++)
            {
                if (buffer[i] == 0) { break; }
            }

            i++;
            if(strncmp(&(buffer[i]), "exit", 4) == 0)
            {
                buffer[0] = 0xFF;
                buffer[3] = 0;
                bytes_tx = sendto(serverSocket, buffer, 4, 0, addressC, clientAddressSize);
                printf("Bytes sended:\n");
                print_buffer(bytes_tx);
                break;
            }

            // send confirm
            buffer[0] = 0x00;
            char oldVal = buffer[3]; // store old value
            buffer[3] = 0;
            bytes_tx = sendto(serverSocket, buffer, 4, 0, addressC, clientAddressSize);
            printf("Bytes sended (%i):\n", bytes_tx);
            print_buffer(bytes_tx);

            // send same message back
            buffer[0] = 0x04;
                // increase counter
            buffer[3] = oldVal; // revert it
            buffer[1] = ((unsigned char) buffer[1]) + 1;
            bytes_tx = sendto(serverSocket, buffer, bytes_rx, 0, addressC, clientAddressSize);
            printf("Bytes sended (%i):\n", bytes_tx);
            print_buffer(bytes_tx);
        }
        else
        {
            if(ignoreClient) {continue;}
            // send confirm
            buffer[0] = 0x00;
            buffer[3] = 0;
            bytes_tx = sendto(serverSocket, buffer, 3, 0, addressC, clientAddressSize);
            printf("Bytes sended (%i):\n", bytes_tx);
            print_buffer(bytes_tx);
        }

        if(bytes_tx < 0)
        {
            fprintf(stderr, "ERROR: sendto\n");
            exit(SENDING_FAILED);
        }
    }

    shutdown(serverSocket, SHUT_RDWR);
}