// #include "netinet/ether.h"
// #include "netinet/tcp.h"
// #include "netinet/udp.h"
// #include "netinet/ip_icmp.h"
#include "arpa/inet.h"
#include "sys/socket.h"
#include "netdb.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "getopt.h"
#include "../tests/clientXserver.h"

// typedef char byte;
#define byte char

int errHandling(const char* msg, int errorCode)
{
    fprintf(stderr, "ERROR: %s\n", msg);
    exit(errorCode);
    return 0;
}

enum Protocols {UDP, TCP};

int getSocket(enum Protocols protocol)
{   
    int family;
    switch (protocol)
    {
        case UDP: family = SOCK_DGRAM; break; 
        case TCP: family = SOCK_STREAM; break;
        default: errHandling("Unknown protocol passed to function get socket)", -1); 
    }

    int newSocket = socket(family, AF_INET, 0);
    if(newSocket < 0) { errHandling("Socket creation failed", 1); /*TODO:*/ }

    return newSocket;
}

void processArguments(int argc, char* argv[])
{
    // TODO: delete 
    if(argc || argv) {}
}

struct sockaddr_in findServer(const char* serverHostname, uint16_t serverPort)
{
    struct hostent* server = gethostbyname(serverHostname);
    if(server == NULL)
    {
        fprintf(stderr, "ERROR: No such host %s\n", serverHostname);
        exit(1); //TODO:
    }

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    memcpy(&serverAddress.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    
    #ifdef DEBUG
    printf("INFO: Server socket %s : %d \n", inet_ntoa(serverAddress.sin_addr), ntohs(serverAddress.sin_port));
    #endif
    
    // struct sockaddr* address = (struct sockaddr*) &serverAddress;
    // int addressSize = sizeof(serverAddress);

    // return address;
    return serverAddress;
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
size_t loadBuffer(char** buffer, size_t* bufferSize)
{
    char c = getc(stdin);
    
    size_t i = 0;
    for(; isEndingCharacter(c) ; i++)
    {
        
        // resize buffer if smaller than num. of loaded characters or zero
        if(*buffer == NULL || *bufferSize <= 0 || (*bufferSize) < i) 
        {
            // Get size of new buffer
            const size_t sizeToAllocate = (*bufferSize <= 0)? 512 : *bufferSize * 2;
            // Realloc buffer
            char* tmp = realloc(*buffer, sizeToAllocate);
            // Check for failed memory reallocation
            if(tmp == NULL)
            {
                fprintf(stderr, "ERROR: Realloc failed");
                exit(1); // TODO: error code change
            }
            // Save new value to buffer and bufferSize
            *bufferSize = sizeToAllocate;
            *buffer = tmp;
        }

        (*buffer)[i] = c;
        c = getc(stdin);
    }

    // Add 0 to the end of string
    (*buffer)[i] = '\0';

    return i;
}


// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

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

typedef struct BytesBlock {
    char* start; // pointer to the starting character of the block
    size_t len; // length of the block
} BytesBlock;


#define MSG_TYPE_CONFIRM 0x00
#define MSG_TYPE_REPLY 0x01
#define MSG_TYPE_AUTH 0x02
#define MSG_TYPE_JOIN 0x03
#define MSG_TYPE_MSG 0x04
#define MSG_TYPE_ERR 0xFE
#define MSG_TYPE_BYE 0xFF

/**
 * @brief Breaks received protocol IPK2024 (from variable buffer) into parts  
 * 
 * @param buffer 
 * @param bufferSize 
 */
/*
void commandLookUp(char* buffer, size_t bufferSize)
{
    // Message type is always first (0th) received byte
    BytesBlock msgType = {.start=&(buffer[0]), .end=&(buffer[1]), .len=1};
    // Message ID is always 1st byte and is long 2 bytes
    BytesBlock msgId = { .start = &(buffer[1]), .end=&(buffer[3]), .len=2};
    BytesBlock first = {NULL, NULL, 0}, second = {NULL, NULL, 0}, third = {NULL, NULL, 0};

    first.start = msgId.end;
    size_t index; // Temporaty helping variable to hold index in string

    uint8_t msgTypeInt = *(msgType.start);
    // uint8_t msgTypeInt = buffer[0];
    switch (msgTypeInt)
    {
    case MSG_TYPE_CONFIRM: break; // No addtional bytes needed
    case MSG_TYPE_REPLY:
        // Reply: Result (1 byte)
        first.end = &(first.start[1]);
        first.len = 1;
        // Reply: Ref_MessageID (2 bytes)
        second.start = first.end;
        second.end = &(second.start[2]);
        second.len = 2;
        // Reply: MessageContents (x bytes)
        third.start = second.end;
        index = findZeroInString(third.start, bufferSize - (msgType.len + msgId.len + first.len + second.len));
        third.end = &(third.start[index]);
        break;
    case MSG_TYPE_AUTH:
        // Auth: Username (x bytes)
        index = findZeroInString(first.start, bufferSize - (msgType.len + msgId.len));
        first.end = &(first.start[index]);
        // Auth: DisplayName (x bytes)
        second.start = &(first.end[1]); // skip one zero byte
        index = findZeroInString(first.start, bufferSize - (msgType.len + msgId.len + first.len));
        second.end = &(second.start[index]);
        // Auth: Secret (x bytes)
        third.start = &(second.end[1]); // skip one zero byte
        index = findZeroInString(first.start, bufferSize - (msgType.len + msgId.len + first.len + second.len));
        third.end = &(third.start[index]);
        break;
    case MSG_TYPE_JOIN:
    case MSG_TYPE_MSG:
    case MSG_TYPE_ERR:
        // MSG, ERR: DisplayName (x bytes) / JOIN: ChannelID (x bytes) 
        index = findZeroInString(first.start, bufferSize - (msgType.len + msgId.len));
        first.end = &(first.start[index]);
        // MSG, ERR: MessageContents (x bytes) / JOIN: DisplayName (x bytes) 
        second.start = &(first.end[1]); // skip one zero byte
        index = findZeroInString(first.start, bufferSize - (msgType.len + msgId.len + first.len));
        second.end = &(second.start[index]);
        break;
    case MSG_TYPE_BYE: break; // No addtional bytes needed
    default:
        fprintf(stderr, "ERROR: Unknown message type\n");
        exit(1); 
        break;
    }
}
*/

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Finds first blank character (spaces ' ' and tabulators '\t') in string and returns index 
 * of last character before blank character
 * 
 * @param string Input string to be searched in
 * @param len Length of string
 * @return long Index in string
 */
long findBlankCharInString(char* string, size_t len)
{
    size_t index = 0;
    for(; index < len ; index++)
    {
        if(string[index] == ' ' || string[index] == '\t' || string[index] == '\0' || string[index] == '\n')
        { 
            return index;
        }
    }

    // Character was not found
    return -1;
}

/**
 * @brief 
 * 
 * @param string 
 * @param len 
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
 * @brief Fills BytesBlock variable from last word found 
 * Skips all blank characters (spaces and tabs) 
 * @param block Variable of initialized 
 * @param startOfLastWord 
 * @param bufferSize 
 */
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

void printByteBlock(BytesBlock* block, int hex)
{
    for(size_t i = 0; i <= block->len; i++)
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

typedef enum CommandType {AUTH, JOIN, RENAME, HELP, PLAIN_MSG} cmd_t;

/**
 * @brief 
 * 
 * @param buffer 
 * @param bufferSize 
 */
cmd_t commandHandler(char* buffer, size_t bufferSize, BytesBlock commnads[4])
{
    // size_t index = findBlankCharInString(buffer, bufferSize);
    BytesBlock cmd = {.start=buffer, .len=0};

    BytesBlock first = {NULL, 0}, second = {NULL, 0}, third = {NULL, 0};

    cmd_t type;

    if(strncmp(cmd.start, "/auth", cmd.len) == 0)
    {
        cmd.len = 5;
        getWord(&first, &(cmd.start[cmd.len]), bufferSize - (cmd.len));
        getWord(&second, &(first.start[cmd.len-1]), bufferSize - (cmd.len + first.len));
        getWord(&third, &(second.start[cmd.len-1]), bufferSize - (cmd.len + first.len + second.len));
        type = AUTH;
    }
    else if(strncmp(cmd.start, "/join", cmd.len) == 0)
    {
        cmd.len = 5;
        getWord(&first, &(cmd.start[cmd.len]), bufferSize - (cmd.len));
        type = JOIN;
    }
    else if(strncmp(cmd.start, "/rename", cmd.len) == 0)
    {
        cmd.len = 7;
        getWord(&first, &(cmd.start[cmd.len]), bufferSize - (cmd.len));
        type = RENAME;
    }
    else if(strncmp(cmd.start, "/help", cmd.len) == 0)
    {
        cmd.len = 5;
        type = HELP;
    }
    else
    {
        type = PLAIN_MSG;
    }

    commnads[0] = cmd;
    commnads[1] = first;
    commnads[2] = second;
    commnads[3] = third;

    return type;
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

void assembleProtocol(cmd_t type, BytesBlock commands[4])
{

    if(type){}
    printByteBlock(&(commands[0]), 0);
    printByteBlock(&(commands[1]), 0);
    printByteBlock(&(commands[2]), 0);
    printByteBlock(&(commands[3]), 0);

    printf("\n");
    printByteBlock(&(commands[0]), 1);
    printByteBlock(&(commands[1]), 1);
    printByteBlock(&(commands[2]), 1);
    printByteBlock(&(commands[3]), 1);
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // ------------------------------------------------------------------------
    // 
    // ------------------------------------------------------------------------

    char serverHostname[] = "127.0.0.1"; /*"anton5.fit.vutbr.cz"*/
    uint16_t serverPort = PORT_NUMBER; /*4567*/ 

    processArguments(argc, argv);

    // ------------------------------------------------------------------------
    // 
    // ------------------------------------------------------------------------

    // open socket for comunication on client side (this)
    int clientSocket = getSocket(UDP);

    // get server address
    struct sockaddr_in address = findServer(serverHostname, serverPort);
    struct sockaddr* serverAddress = (struct sockaddr*) &address;
    int addressSize = sizeof(address);

    // ------------------------------------------------------------------------
    // 
    // ------------------------------------------------------------------------

    int flags = 0; //TODO: check if some usefull flags could be done
    char* buffer = NULL; // variable to store client input to be send to server
    size_t bufferSize = 0; // size of buffer to correctly manage memory
    unsigned numOfMsgs = 0; // counter of messages send
    cmd_t cmdType; // variable to store current command typed by user

    BytesBlock commands[4];
    do 
    {
        size_t bytesLoaded = loadBuffer(&buffer, &bufferSize);
        cmdType = commandHandler(buffer, bytesLoaded, commands);

        assembleProtocol(cmdType, commands);
        // send buffer to the server 
        int bytesTx = sendto(clientSocket, buffer, bytesLoaded, flags, serverAddress, addressSize);

        if(bytesTx < 0)
        {
            fprintf(stderr, "ERROR: Sending was not successful\n");
            exit(1); // TODO: error code change
        }

        // increase number of messages recevied
        numOfMsgs++;
    } while (strcmp(buffer, "/exit") != 0);

    printf("Communicaton ended with %u messages\n", numOfMsgs); //TODO: delete

    // ------------------------------------------------------------------------
    // Closing up communication
    // ------------------------------------------------------------------------

    // close clientSocket
    shutdown(clientSocket, SHUT_RDWR);
    free(buffer);

    return 0;
}