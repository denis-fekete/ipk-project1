/**
 * @file ipk24protocol.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "ipk24protocol.h"

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

#define BLOCK_TO_BUFF(dst, src) stringReplace(&(dst), src.start, src.len)

/**
 * @brief Assembles protocol from commands and command type into a buffer
 * 
 * @param type Recognized type of command user provided
 * @param commands Separated commands from user input
 * @param buffer Output buffer to be trasmited to the server
 * that don't have all informations provided by user at start
 * @param progInt Pointer to ProgramInterface
 * 
 * @return Returns true if buffer can be trasmitted to the server
 */
bool assembleProtocol(cmd_t type, BytesBlock commands[4], Buffer* buffer, ProgramInterface* progInt)
{
    size_t expectedSize = commands[0].len + commands[1].len + commands[2].len + commands[3].len + 10;
    bufferResize(buffer, expectedSize);

    size_t ptrPos = 0;

    // Break msgCounter into two bites
    char high;
    char low;
    breakMsgIdToBytes(&high, &low, progInt->comDetails->msgCounter);

    // MessageType
    switch (type)
    {
    case cmd_AUTH: buffer->data[0] = msg_AUTH; break;
    case cmd_JOIN: buffer->data[0] = msg_JOIN; break;
    case cmd_RENAME: buffer->data[0] = msg_JOIN; break;
    case cmd_MSG: buffer->data[0] = msg_MSG; break;
    case cmd_CONF: 
        buffer->data[0] = msg_CONF;
        buffer->data[1] = *(commands[0].start);
        buffer->data[2] = *(commands[1].start);
        buffer->used = 3;
        return true;
        break;
    case cmd_EXIT: 
        buffer->data[0] = (unsigned char) msg_BYE;
        buffer->data[1] = high;
        buffer->data[2] = low;
        buffer->used = 3;
        return true; 
        break;
    default: errHandling("Unknown command type in assembleProtocol() function", 1) /*TODO: change error code*/; break;
    }
    ptrPos += 1;

    // MessageID
    buffer->data[1] = high;  
    ptrPos += 1;
    buffer->data[2] = low;  
    ptrPos += 1;

    // ------------------------------------------------------------------------
    if(type == cmd_AUTH || type == cmd_JOIN)
    {
        // Join: ChannelID  / Auth: Username
        BLOCK_TO_BUFF(buffer->data[ptrPos], commands[1]);
        ptrPos += commands[1].len;
        // 0 byte
        buffer->data[ptrPos] = 0;
        ptrPos += 1;
    }
    else if(type == cmd_RENAME)
    {
        if(progInt->comDetails->channelID.data == NULL)
        { 
            safePrintStdout("System: ChannelID not provided, cannot rename! (Did you use /auth before this commands?). Use /help for help.\n");
            return false;
        }
        // Rename: ChannelID
        stringReplace(&( buffer->data[ptrPos] ), progInt->comDetails->channelID.data,
                progInt->comDetails->channelID.used);
        ptrPos += progInt->comDetails->channelID.used;
        // 0 byte
        buffer->data[ptrPos] = 0;
        ptrPos += 1;
    }
    else if(type == cmd_MSG)
    {
        if(progInt->comDetails->displayName.data == NULL)
        { 
            safePrintStdout("System: ChannelID not provided, cannot rename! (Did you use /auth before this commands?). Use /help for help.\n");
            return false;
        }
        // Rename: ChannelID
        stringReplace(&( buffer->data[ptrPos] ), progInt->comDetails->displayName.data,
            progInt->comDetails->displayName.used);
        ptrPos += progInt->comDetails->displayName.used;
        // 0 byte
        buffer->data[ptrPos] = 0;
        ptrPos += 1;
    }

    // ------------------------------------------------------------------------
    if(type == cmd_AUTH || type == cmd_MSG || type == cmd_RENAME) // Msg: MessageContents / Auth: DisplayName
    {
        if(type == cmd_AUTH)
        {
            BLOCK_TO_BUFF(buffer->data[ptrPos], commands[2]);
            ptrPos += commands[2].len;
        }
        else
        {
            BLOCK_TO_BUFF(buffer->data[ptrPos], commands[1]);
            ptrPos += commands[1].len;
        }
        // 0 byte
        buffer->data[ptrPos] = 0;
        ptrPos += 1;
    } 
    else if (type == cmd_JOIN) // Join: DisplayName
    {
        if(progInt->comDetails->displayName.data == NULL)
        { 
            safePrintStdout("Displayname not provided, cannot join! Use /help for help.");
            return false;
        }

        stringReplace(&( buffer->data[ptrPos] ), progInt->comDetails->displayName.data,
            progInt->comDetails->displayName.used);
        ptrPos += progInt->comDetails->displayName.used;
        // 0 byte
        buffer->data[ptrPos] = 0;
        ptrPos += 1;
    }
    // ------------------------------------------------------------------------
    if(type == cmd_AUTH)
    {
        // Auth: Secret
        BLOCK_TO_BUFF(buffer->data[ptrPos], commands[3]);
        ptrPos += commands[3].len;
        // 0 byte
        buffer->data[ptrPos] = 0;
        ptrPos += 1;
    }
    // ------------------------------------------------------------------------

    buffer->used = ptrPos;

    return true;
}
#undef BLOCK_TO_BUFF

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Converts 16bit message id into an two unsinged chars
 * 
 * @param high Output pointer to unsigned char, upper/higher half of number
 * @param low Output pointer to unsigned char, lower half of number
 * @param msgCounter Input number to be separated
 */
void breakMsgIdToBytes(char* high, char* low, uint16_t msgCounter)
{
    *high = (unsigned char)((msgCounter) >> 8);
    *low = (unsigned char)((msgCounter) & 0xff);
}

/**
 * @brief Converts bwo bytes from input char array into 16bit usigned integer
 * 
 * @param buffer Input char array
 * @return u_int16_t 
 */
u_int16_t convert2BytesToUInt(char* input)
{
    // Get msgId
    unsigned char high = input[1];
    unsigned char low = input[2];

    // Join bytes into one number
    input[1] = high;  
    input[2] = low;  
    return (low +  (high << 8)); 
}


#define BBLOCK_END(block) &(block.start[block.len]);
#define BBLOCK_END_W_ZERO_BYTE(block) &(block.start[block.len + 1]);
#define TYPE_ID_LEN 3

/**
 * @brief 
 * 
 * @param buffer 
 * @param commands 
 * @param msgType 
 * @param msgId 
 */
void disassebleProtocol(Buffer* buffer, BytesBlock commands[4], msg_t* msgType, u_int16_t* msgId)
{
    size_t index; // Temporaty helping variable to hold index in string
    BytesBlock first = {NULL, 0}, second = {NULL, 0}, third = {NULL, 0};

    // Get msg type
    u_int8_t msgTypeInt = (u_int8_t) (buffer->data[0]);
    
    *msgId = convert2BytesToUInt(buffer->data);
    
    first.start = &(buffer->data[3]);
    // ------------------------------------------------------------------------

    switch (msgTypeInt)
    {
    case msg_CONF: break; // No addtional bytes needed
    case msg_REPLY:
        // Reply: Result (1 byte)
        first.len = 1;
        // Reply: Ref_MessageID (2 bytes)
        second.start = BBLOCK_END(first);
        second.len = 2;
        // Reply: MessageContents (x bytes)
        third.start = BBLOCK_END(second);
        index = findZeroInString(third.start, buffer->used - (TYPE_ID_LEN + first.len + second.len));
        third.len = index;
        break;
    // ------------------------------------------------------------------------
    case msg_AUTH:
        // Auth: Username (x bytes)
        index = findZeroInString(first.start, buffer->used - (TYPE_ID_LEN));
        first.len = index;
        // Auth: DisplayName (x bytes)
        second.start = BBLOCK_END_W_ZERO_BYTE(first); // skip one zero byte
        index = findZeroInString(first.start, buffer->used - (TYPE_ID_LEN + first.len));
        second.len = index;
        // Auth: Secret (x bytes)
        third.start = BBLOCK_END_W_ZERO_BYTE(second); // skip one zero byte
        index = findZeroInString(first.start, buffer->used - (TYPE_ID_LEN + first.len + second.len));
        third.len = index;
        break;
    // ------------------------------------------------------------------------
    case msg_JOIN:
    case msg_MSG:
    case msg_ERR:
        // MSG, ERR: DisplayName (x bytes) / cmd_JOIN: ChannelID (x bytes) 
        index = findZeroInString(first.start, buffer->used - (TYPE_ID_LEN));
        first.len = index;
        // MSG, ERR: MessageContents (x bytes) / cmd_JOIN: DisplayName (x bytes) 
        second.start = BBLOCK_END_W_ZERO_BYTE(first) // skip one zero byte
        index = findZeroInString(first.start, buffer->used - (TYPE_ID_LEN + first.len));
        second.len = index;
        break;
    // ------------------------------------------------------------------------
    case msg_BYE: break; // No addtional bytes needed
    default:
        fprintf(stderr, "ERROR: Unknown message type\n");
        exit(1); 
        break;
    }

    *msgType = (enum MessageType) msgTypeInt;
    commands[0] = first;
    commands[1] = second;
    commands[2] = third;
}
#undef BBLOCK_END
#undef BBLOCK_END_W_ZERO_BYTE


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
cmd_t userInputToCmds(Buffer* buffer, BytesBlock commands[4], bool* eofDetected)
{
    // If buffer is not filled skip
    if(buffer->used <= 0) { return cmd_NONE; }

    size_t index = findBlankCharInString(buffer->data, buffer->used);
    BytesBlock cmd = {.start=buffer->data, .len=index};

    BytesBlock first = {NULL, 0}, second = {NULL, 0}, third = {NULL, 0};

    cmd_t type;

    if(*eofDetected)
    {
        type = cmd_EXIT;
    }
    else if(strncmp(cmd.start, "/auth", cmd.len) == 0)
    {
        getWord(&first, &(cmd.start[cmd.len]), buffer->used - (cmd.len));
        getWord(&second, &(first.start[first.len]), buffer->used - (cmd.len + first.len));
        getWord(&third, &(second.start[second.len]), buffer->used - (cmd.len + first.len + second.len));
        type = cmd_AUTH;
    }
    else if(strncmp(cmd.start, "/join", cmd.len) == 0)
    {
        getWord(&first, &(cmd.start[cmd.len]), buffer->used - (cmd.len));
        type = cmd_JOIN;
    }
    else if(strncmp(cmd.start, "/rename", cmd.len) == 0)
    {
        getWord(&first, &(cmd.start[cmd.len]), buffer->used - (cmd.len));
        type = cmd_RENAME;
    }
    else if(strncmp(cmd.start, "/help", cmd.len) == 0)
    {
        type = cmd_HELP;
    }
    else if(strncmp(cmd.start, "/exit", cmd.len) == 0)
    {
        type = cmd_EXIT;
    }
    else
    {
        first.start = buffer->data;
        first.len = buffer->used; 
        type = cmd_MSG;
    }

    commands[0] = cmd;
    commands[1] = first;
    commands[2] = second;
    commands[3] = third;

    return type;
}
