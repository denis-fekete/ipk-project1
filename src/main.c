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

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Processes arguments provided by user
 * 
 * @param argc Number of arguments given
 * @param argv Array of arguments strings (char pointers)
 */
void processArguments(int argc, char* argv[])
{
    // TODO: delete 
    if(argc || argv) {}
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Takes input from user (client) from buffer and break it into an
 * array of commands (ByteBlock)
 * 
 * @warning Commands contain only pointers to the buffer, rewriting buffer
 * will lead to unexpected behavior commands in commands
 * 
 * @param buffer Input buffer containing command from user (client)
 * @param commands Array of commands where separated commands will be store 
 * @return Returns type of recognized command (enum CommandType)
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

/**
 * @brief Updates values in CommunicationDetails comDetails variable
 * based on cmdType provided.  
 * 
 * @note Stored data is copied, so commands can be safely rewritten
 * 
 * @param cmdType Recognized command from user 
 * @param commands Array of commands
 * @param comDetails Pointer to the CommunicationDetails variable that will 
 * be updated
 */
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

#define BLOCK_TO_BUFF(dst, src) stringReplace(&(dst), src.start, src.len)
/**
 * @brief Assembles 
 * 
 * @param type Recognized type of command user provided
 * @param commands Separated commands from user input
 * @param buffer Output buffer to be trasmited to the server
 * @param comDetails CommunicationDetails need by some commands like JOIN 
 * that don't have all informations provided by user at start
 * 
 * @return Returns true if buffer can be trasmitted to the server
 */
bool assembleProtocol(cmd_t type, BytesBlock commands[4], Buffer* buffer, CommunicationDetails* comDetails)
{
    size_t expectedSize = commands[0].len + commands[1].len + commands[2].len + commands[3].len + 10;
    bufferResize(buffer, expectedSize);

    size_t ptrPos = 0;

    // MessageType
    switch (type)
    {
    case AUTH: buffer->data[0] = MSG_TYPE_AUTH; break;
    case JOIN: buffer->data[0] = MSG_TYPE_JOIN; break;
    case RENAME: buffer->data[0] = MSG_TYPE_JOIN; break;
    default: errHandling("Unknown command type in assembleProtocol() function", 1) /*TODO: change error code*/; break;
    }
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
        BLOCK_TO_BUFF(buffer->data[ptrPos], commands[1]);
        ptrPos += commands[1].len;
        // 0 byte
        buffer->data[ptrPos] = '\0';
        ptrPos += 1;
    }
    else if(type == RENAME)
    {
        if(comDetails->channelID.data == NULL)
        { 
            displayMsgToUser("ChannelID not provided, cannot rename! (Did you use /auth before this commands?). Use /help for help.");
            return false;
        }
        // Rename: ChannelID
        stringReplace(&( buffer->data[ptrPos] ), comDetails->channelID.data, comDetails->channelID.used);
        ptrPos += comDetails->channelID.used;
        // 0 byte
        buffer->data[ptrPos] = '\0';
        ptrPos += 1;
    }

    // ------------------------------------------------------------------------
    if(type == AUTH || type == PLAIN_MSG || type == RENAME) // Msg: MessageContents / Auth: DisplayName
    {
        if(type == RENAME)
        {
            BLOCK_TO_BUFF(buffer->data[ptrPos], commands[1]);
        }
        else
        {
            BLOCK_TO_BUFF(buffer->data[ptrPos], commands[2]);
        }
        ptrPos += commands[2].len;
        // 0 byte
        buffer->data[ptrPos] = '\0';
        ptrPos += 1;
    } 
    else if (type == JOIN) // Join: DisplayName
    {
        if(comDetails->displayName.data == NULL)
        { 
            displayMsgToUser("Displayname not provided, cannot join! Use /help for help.");
            return false;
        }

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
        BLOCK_TO_BUFF(buffer->data[ptrPos], commands[3]);
        ptrPos += commands[3].len;
        // 0 byte
        buffer->data[ptrPos] = '\0';
        ptrPos += 1;
    }
    // ------------------------------------------------------------------------

    buffer->used = ptrPos;

    printBuffer(buffer, 1, 0);
    printBuffer(buffer, 0, 1);

    return true;
}
#undef BLOCK_TO_BUFF

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // ------------------------------------------------------------------------
    // Process arguments from user
    // ------------------------------------------------------------------------

    char serverHostname[] = "127.0.0.1"; /*"anton5.fit.vutbr.cz"*/
    uint16_t serverPort = PORT_NUMBER; /*4567*/ 

    processArguments(argc, argv);

    // ------------------------------------------------------------------------
    // Get server information and create socket
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
    Buffer clientCommands; bufferReset(&clientCommands);
    Buffer protocolMsg; bufferReset(&protocolMsg);
    
    CommunicationDetails comDetails;
    comDetails.msgCounter = 0;
    bufferReset(&(comDetails.displayName));
    bufferReset(&(comDetails.channelID));

    BytesBlock commands[4];

    // ------------------------------------------------------------------------
    // Loop of communication
    // ------------------------------------------------------------------------

    do 
    {
        // Load buffer from stdin, store length of buffer
        clientCommands.used = loadBufferFromStdin(&clientCommands);
        // Separate clientCommands buffer into commands (ByteBlocks),
        // store recognized command
        cmdType = commandHandler(&clientCommands, commands);
        // Based on command type store commands into comDetails for future use
        // in other commands
        storeInformation(cmdType, commands, &comDetails);
        // Assembles array of bytes into Buffer protocolMsg, returns if 
        // message can be trasmitted   
        bool canBeSended = assembleProtocol(cmdType, commands, &protocolMsg, &comDetails);

        int bytesTx; // number of sended bytes
        if(canBeSended)
        {
            // send buffer to the server 
            bytesTx = sendto(clientSocket, clientCommands.data, clientCommands.used, flags, serverAddress, addressSize);

            if(bytesTx < 0)
            {
                errHandling("Sending bytes was not successful", 1); // TODO: change error code
            }
        }
        // increase number of messages recevied
        comDetails.msgCounter += 1;

        // Exits loop if /exit detected 
        if(cmdType == CMD_EXIT) { break; }
    } while (1);

    printf("DEBUG: Communicaton ended with %u messages\n", comDetails.msgCounter); //DEBUG: delete

    // ------------------------------------------------------------------------
    // Closing up communication
    // ------------------------------------------------------------------------

    shutdown(clientSocket, SHUT_RDWR);
    free(comDetails.displayName.data);
    free(comDetails.channelID.data);
    free(clientCommands.data);
    free(protocolMsg.data);

    return 0;
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
