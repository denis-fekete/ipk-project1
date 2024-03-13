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

bool assembleProtocol(cmd_t type, BytesBlock commands[4], Buffer* buffer, CommunicationDetails* comDetails)
{
    size_t expectedSize = commands[0].len + commands[1].len + commands[2].len + commands[3].len + 10;
    bufferResize(buffer, expectedSize);

    size_t ptrPos = 0;

    // MessageType
    switch (type)
    {
    case AUTH: buffer->data[0] = MSG_AUTH; break;
    case JOIN: buffer->data[0] = MSG_JOIN; break;
    case RENAME: buffer->data[0] = MSG_JOIN; break;
    case PLAIN_MSG: buffer->data[0] = MSG_MSG; break;
    case CMD_EXIT: 
        buffer->data[0] = (char) MSG_BYE;
        return true; break;
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
    if(type == AUTH || type == JOIN)
    {
        // Join: ChannelID  / Auth: Username
        BLOCK_TO_BUFF(buffer->data[ptrPos], commands[1]);
        ptrPos += commands[1].len;
        // 0 byte
        buffer->data[ptrPos] = 0;
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
        buffer->data[ptrPos] = 0;
        ptrPos += 1;
    }
    else if(type == PLAIN_MSG)
    {
        if(comDetails->displayName.data == NULL)
        { 
            displayMsgToUser("ChannelID not provided, cannot rename! (Did you use /auth before this commands?). Use /help for help.");
            return false;
        }
        // Rename: ChannelID
        stringReplace(&( buffer->data[ptrPos] ), comDetails->displayName.data, comDetails->displayName.used);
        ptrPos += comDetails->displayName.used;
        // 0 byte
        buffer->data[ptrPos] = 0;
        ptrPos += 1;
    }

    // ------------------------------------------------------------------------
    if(type == AUTH || type == PLAIN_MSG || type == RENAME) // Msg: MessageContents / Auth: DisplayName
    {
        if(type == AUTH)
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
        buffer->data[ptrPos] = 0;
        ptrPos += 1;
    }
    // ------------------------------------------------------------------------
    if(type == AUTH)
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
\
    // Get msgId
    unsigned char high = buffer->data[1];
    unsigned char low = buffer->data[2];

    // Join bytes into one number
    buffer->data[1] = high;  
    buffer->data[2] = low;  
    *msgId = low +  (high << 8); 

    printf("DEBUG: Received message type: %i\n", msgTypeInt);
    printf("DEBUG: Message ID: %u\n", *msgId);

    first.start = &(buffer->data[3]);
    // ------------------------------------------------------------------------

    switch (msgTypeInt)
    {
    case MSG_CONF: break; // No addtional bytes needed
    case MSG_REPLY:
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
    case MSG_AUTH:
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
    case MSG_JOIN:
    case MSG_MSG:
    case MSG_ERR:
        // MSG, ERR: DisplayName (x bytes) / JOIN: ChannelID (x bytes) 
        index = findZeroInString(first.start, buffer->used - (TYPE_ID_LEN));
        first.len = index;
        // MSG, ERR: MessageContents (x bytes) / JOIN: DisplayName (x bytes) 
        second.start = BBLOCK_END_W_ZERO_BYTE(first) // skip one zero byte
        index = findZeroInString(first.start, buffer->used - (TYPE_ID_LEN + first.len));
        second.len = index;
        break;
    // ------------------------------------------------------------------------
    case MSG_BYE: break; // No addtional bytes needed
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
cmd_t userInputToCmds(Buffer* buffer, BytesBlock commands[4])
{
    // If buffer is not filled skip
    if(buffer->used <= 0) { return NONE; }

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
        first.start = buffer->data;
        first.len = buffer->used; 
        type = PLAIN_MSG;
    }

    commands[0] = cmd;
    commands[1] = first;
    commands[2] = second;
    commands[3] = third;

    return type;
}
