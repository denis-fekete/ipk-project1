// #include "netinet/ether.h"
// #include "netinet/tcp.h"
// #include "netinet/udp.h"
// #include "netinet/ip_icmp.h"



#include "getopt.h"
#include "../tests/clientXserver.h"

#include "customtypes.h"
#include "buffer.h"
#include "utils.h"
#include "networkCom.h"



void processArguments(int argc, char* argv[])
{
    // TODO: delete 
    if(argc || argv) {}
}


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
 * @brief 
 * 
 * @param buffer 
 * @param bufferSize 
 */
cmd_t commandHandler(Buffer* buffer, BytesBlock commands[4])
{
    size_t index = findBlankCharInString(buffer->data, buffer->used);
    BytesBlock cmd = {.start=buffer->data, .len=index};

    BytesBlock first = {NULL, 0}, second = {NULL, 0}, third = {NULL, 0};

    cmd_t type;

    if(strncmp(cmd.start, "/auth", cmd.len) == 0)
    {
        getWord(&first, &(cmd.start[cmd.len]), buffer->used - (cmd.len));
        getWord(&second, &(first.start[first.len]), buffer->used - (cmd.len + first.len));
        getWord(&third, &(second.start[second.len]), buffer->used - (cmd.len + first.len + second.len));
        type = AUTH;
    }
    else if(strncmp(cmd.start, "/join", cmd.len) == 0)
    {
        getWord(&first, &(cmd.start[cmd.len]), buffer->used - (cmd.len));
        type = JOIN;
    }
    else if(strncmp(cmd.start, "/rename", cmd.len) == 0)
    {
        getWord(&first, &(cmd.start[cmd.len]), buffer->used - (cmd.len));
        type = RENAME;
    }
    else if(strncmp(cmd.start, "/help", cmd.len) == 0)
    {
        type = HELP;
    }
    else if(strncmp(cmd.start, "/exit", cmd.len) == 0)
    {
        type = CMD_EXIT;
    }
    else
    {
        type = PLAIN_MSG;
    }

    commands[0] = cmd;
    commands[1] = first;
    commands[2] = second;
    commands[3] = third;

    return type;
}

void storeInformation(cmd_t cmdType, BytesBlock commands[4], CommunicationDetails* comDetails)
{
    switch (cmdType)
    {
    case AUTH:
        // commands: CMD, USERNAME, SECRET, DISPLAYNAME
        bufferResize(&(comDetails->displayName), commands[3].len + 1);
        stringReplace(comDetails->displayName.data, commands[3].start, commands[3].len);
        comDetails->displayName.data[commands[3].len] = '\0';
        comDetails->displayName.used = commands[3].len;
        break;
    case JOIN:
        // commands: CMD, CHANNELID
        bufferResize(&(comDetails->channelID), commands[1].len + 1);
        stringReplace(comDetails->channelID.data, commands[1].start, commands[1].len);
        comDetails->channelID.used = commands[1].len;
        break;
    case RENAME:
        // commands: CMD, DISPLAYNAME
        bufferResize(&(comDetails->displayName), commands[1].len + 1);
        stringReplace(comDetails->displayName.data, commands[1].start, commands[1].len);
        comDetails->displayName.data[commands[1].len] = '\0';
        comDetails->displayName.used = commands[1].len;
        break;;
    default: break;
    }
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

#define COPY_STR_TO_BUFFER(dst, src) stringReplace(&(dst), src.start, src.len)

void assembleProtocol(cmd_t type, BytesBlock commands[4], Buffer* buffer, CommunicationDetails* comDetails)
{
    size_t expectedSize = commands[0].len + commands[1].len + commands[2].len + commands[3].len + 10;
    bufferResize(buffer, expectedSize);

    size_t ptrPos = 0;

    // MessageType
    buffer->data[0] = MSG_TYPE_AUTH;
    ptrPos += 1;

    // MessageID
    // Break msgCounter into two bites
    unsigned char high = (unsigned char)((comDetails->msgCounter) >> 8);
    unsigned char low = (unsigned char)((comDetails->msgCounter) & 0xff);

    buffer->data[1] = high;  
    ptrPos += 1;
    buffer->data[2] = low;  
    ptrPos += 1;

    // ------------------------------------------------------------------------
    if(type == AUTH || type == JOIN || type == PLAIN_MSG)
    {
        // Join/Msg: ChannelID / Auth: Username
        COPY_STR_TO_BUFFER(buffer->data[ptrPos], commands[1]);
        ptrPos += commands[1].len;
        // 0 byte
        buffer->data[ptrPos] = '\0';
        ptrPos += 1;
    }

    // ------------------------------------------------------------------------
    if(type == AUTH || type == PLAIN_MSG)
    {
        // Join: DisplayName / Msg: MessageContents / Auth: DisplayName
        COPY_STR_TO_BUFFER(buffer->data[ptrPos], commands[2]);
        ptrPos += commands[2].len;

        // 0 byte
        buffer->data[ptrPos] = '\0';
        ptrPos += 1;
    } 
    else if (type == JOIN)
    {
        stringReplace(&( buffer->data[ptrPos] ), comDetails->displayName.data, comDetails->displayName.used);
        ptrPos += comDetails->displayName.used;

        // 0 byte
        buffer->data[ptrPos] = '\0';
        ptrPos += 1;
    }
    // ------------------------------------------------------------------------
    if(type == AUTH)
    {
        // Auth: Secret
        COPY_STR_TO_BUFFER(buffer->data[ptrPos], commands[3]);
        ptrPos += commands[3].len;
        // 0 byte
        buffer->data[ptrPos] = '\0';
        ptrPos += 1;
    }
    // ------------------------------------------------------------------------

    buffer->used = ptrPos;

    printBuffer(buffer, 1, 0);
    printBuffer(buffer, 0, 1);
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
    // Declaring and initializing variables need for communication
    // ------------------------------------------------------------------------

    int flags = 0; //TODO: check if some usefull flags could be done
    cmd_t cmdType; // variable to store current command typed by user
    Buffer clientCommands;
    Buffer protocolMsg;

    bufferReset(&clientCommands);
    bufferReset(&protocolMsg);
    
    CommunicationDetails comDetails;
    comDetails.msgCounter = 0;

    bufferReset(&(comDetails.displayName));
    bufferReset(&(comDetails.channelID));

    // ------------------------------------------------------------------------
    // 
    // ------------------------------------------------------------------------

    BytesBlock commands[4];
    do 
    {
        // size_t bytesLoaded = loadBuffer(&buffer, &bufferSize);
        clientCommands.used = loadBufferFromStdin(&clientCommands);

        cmdType = commandHandler(&clientCommands, commands);

        storeInformation(cmdType, commands, &comDetails);

        assembleProtocol(cmdType, commands, &protocolMsg, &comDetails);

        // send buffer to the server 
        int bytesTx = sendto(clientSocket, clientCommands.data, clientCommands.used, flags, serverAddress, addressSize);

        if(bytesTx < 0)
        {
            fprintf(stderr, "ERROR: Sending was not successful\n");
            exit(1); // TODO: error code change
        }

        if(cmdType == CMD_EXIT) { break; }

        // increase number of messages recevied
        comDetails.msgCounter += 1;
    } while (1);

    printf("Communicaton ended with %u messages\n", comDetails.msgCounter); //TODO: delete

    // ------------------------------------------------------------------------
    // Closing up communication
    // ------------------------------------------------------------------------

    // close clientSocket
    shutdown(clientSocket, SHUT_RDWR);
    free(clientCommands.data);
    free(protocolMsg.data);

    return 0;
}