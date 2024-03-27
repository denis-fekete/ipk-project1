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
#define TYPE_PLUS_MSG_ID 3
#define ZERO_BYTE 1

/**
 * @brief Assembles protocol from commands and command type into a buffer
 * 
 * @param type Recognized type of command user provided
 * @param commands Separated commands from user input
 * @param buffer Output buffer to be trasmited to the server
 * that don't have all informations provided by user at start
 * @param progInt Pointer to ProgramInterface
 * 
 * @return Returns true if buffer can be sended to the server
 */
bool assembleProtocol(cmd_t type, BytesBlock commands[4], Buffer* buffer, ProgramInterface* progInt)
{
    size_t expectedSize;

    switch(type)
    {
        case cmd_AUTH:
            expectedSize =  TYPE_PLUS_MSG_ID +
                commands[1].len + ZERO_BYTE + 
                commands[3].len + ZERO_BYTE +
                commands[2].len + ZERO_BYTE;
            break;
        case cmd_RENAME: return false; break;
        case cmd_JOIN:
            expectedSize = TYPE_PLUS_MSG_ID +
                progInt->comDetails->channelID.used + ZERO_BYTE +
                progInt->comDetails->displayName.used + ZERO_BYTE;
            break;
        case cmd_MSG:
            expectedSize = TYPE_PLUS_MSG_ID +
                progInt->comDetails->displayName.used + ZERO_BYTE +
                commands[1].len + ZERO_BYTE;
            break;
        case cmd_EXIT:
        case cmd_CONF:
            expectedSize = TYPE_PLUS_MSG_ID;
            break;
        default: 
        printf("%i\n", type);
        errHandling("Unexpected command type in assembleProtocol()\n", 1);
    }

    bufferResize(buffer, expectedSize);
    // bufferResize(buffer, 1500);

    size_t ptrPos = 0;

    // MessageType
    switch (type)
    {
    case cmd_AUTH: buffer->data[0] = msg_AUTH; break;
    case cmd_JOIN: buffer->data[0] = msg_JOIN; break;
    // case cmd_RENAME: buffer->data[0] = msg_JOIN; break; // TODO:delete?
    case cmd_MSG: buffer->data[0] = msg_MSG; break;
    case cmd_CONF: 
        buffer->data[0] = msg_CONF;
        buffer->data[1] = commands[0].start[0];
        buffer->data[2] = commands[1].start[0];
        buffer->used = 3;
        return true;
        break;
    case cmd_EXIT: 
        buffer->data[0] = (unsigned char) msg_BYE;
        // buffer->data[1] = high;
        // buffer->data[2] = low;
        buffer->used = 3;
        return true; 
        break;
    default: errHandling("Unknown command type in assembleProtocol() function", 1) /*TODO: change error code*/; break;
    }
    ptrPos += 1;

    // MessageID
    // buffer->data[1] = high;  
    ptrPos += 1;
    // buffer->data[2] = low;  
    ptrPos += 1;

    // ------------------------------------------------------------------------
    // First block
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
    /*else if(type == cmd_RENAME)
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
    }*/
    else if(type == cmd_MSG)
    {
        if(progInt->comDetails->displayName.data == NULL)
        { 
            safePrintStdout("System: ChannelID not provided, cannot rename! (Did you use /auth before this commands?). Use /help for help.\n");
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
    // Second block
    // ------------------------------------------------------------------------

    if(type == cmd_AUTH || type == cmd_MSG) // Msg: MessageContents / Auth: DisplayName
    {
        if(type == cmd_AUTH)
        {
            BLOCK_TO_BUFF(buffer->data[ptrPos], commands[3]);
            ptrPos += commands[3].len;
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
    else if (type == cmd_JOIN || type == cmd_RENAME) // Join: DisplayName
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
    // Third block
    // ------------------------------------------------------------------------


    if(type == cmd_AUTH)
    {
        // Auth: Secret
        BLOCK_TO_BUFF(buffer->data[ptrPos], commands[2]);
        ptrPos += commands[2].len;
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
void breakU16IntToBytes(char* high, char* low, uint16_t msgCounter)
{
    *high = (unsigned char)((msgCounter) >> 8);
    *low = (unsigned char)((msgCounter) & 0xff);
}

/**
 * @brief Converts bwo bytes from input char array into 16bit usigned integer
 * 
 * @param high Higher byte
 * @param low Lower byte
 * @return u_int16_t 
 */
u_int16_t convert2BytesToU16Int(char low, char high)
{
    // Join bytes into one number
    return (low +  (high << 8)); 
}

/**
 * @brief Converts integer into and MessageType
 * 
 * @param input Input integer
 * @return msg_t MessageType to be returned
 */
msg_t uchar2msgType(unsigned char input)
{
    switch (input)
    {
    case 0x00: return msg_CONF;
    case 0x01: return msg_REPLY;
    case 0x02: return msg_AUTH;
    case 0x03: return msg_JOIN;
    case 0x04: return msg_MSG;
    case 0xFE: return msg_ERR;
    case 0xFF: return msg_BYE;
    default: return msg_UNKNOWN;
    }
}

#define BBLOCK_END(block) &(block.start[block.len]);
#define BBLOCK_END_W_ZERO_BYTE(block) &(block.start[block.len + 1]);
#define TYPE_ID_LEN 3

/**
 * @brief Dissassembles protocol from Buffer into commands, msgType and msgId
 * 
 * @param buffer Input buffer containing message
 * @param commands Array of BytesBlocks pointing to values
 * @param msgType Detected message type
 * @param msgId Detected message ID 
 */
void disassebleProtocol(Buffer* buffer, BytesBlock commands[4], msg_t* msgType, u_int16_t* msgId)
{
    size_t index; // Temporaty helping variable to hold index in string
    BytesBlock first = {NULL, 0}, second = {NULL, 0}, third = {NULL, 0};

    // Get msg type
    *msgType = uchar2msgType( (unsigned char) buffer->data[0] );
    
    *msgId = convert2BytesToU16Int(buffer->data[1], buffer->data[2]);
    
    first.start = &(buffer->data[3]);
    // ------------------------------------------------------------------------

    switch (*msgType)
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
        // Auth: Secret (x bytes)
        second.start = BBLOCK_END_W_ZERO_BYTE(first); // skip one zero byte
        index = findZeroInString(first.start, buffer->used - (TYPE_ID_LEN + first.len));
        second.len = index;
        // Auth: DisplayName (x bytes)
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
        debugPrint(stdout, "Unknown message type: %i\n", (int)*msgType);
        errHandling("ERROR: Protocol disassembler received unknown message type", 1); // TODO:change
    }

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
 * @param eofDetected Signals that end of file was detected
 * @param flags Flags that will be set to message
 * @return cmd_t Returns command type
 */
cmd_t userInputToCmds(Buffer* buffer, BytesBlock commands[4], bool* eofDetected, msg_flags* flags)
{
    // If buffer is not filled skip
    if(buffer->used <= 0) { return cmd_NONE; }

    size_t index = findBlankCharInString(buffer->data, buffer->used);
    BytesBlock cmd = {.start=buffer->data, .len=index};

    BytesBlock first = {NULL, 0}, second = {NULL, 0}, third = {NULL, 0};

    cmd_t type = cmd_NONE;

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
        *flags = msg_flag_AUTH;
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
        cmd.len = 0;
        cmd.start = NULL;

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