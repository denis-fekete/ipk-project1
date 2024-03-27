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

#define ADD_BLOCK_TO_BUFFER(dst, src)           \
    stringReplace(&(dst), src.start, src.len);  \
    ptrPos += src.len;                          \
    /*add to zero byte*/                        \
    buffer->data[ptrPos] = 0;                   \
    ptrPos += 1;

#define ADD_STORED_INFO_TO_BUFFER(dst, src)     \
    stringReplace(&(dst), src.data, src.used);  \
    ptrPos += src.used;                         \
    /*add to zero byte*/                        \
    buffer->data[ptrPos] = 0;                   \
    ptrPos += 1;

/**
 * @brief Assembles protocol from commands and command type into a buffer
 * 
 * @param pBlocks Separated commands and values from user input
 * @param buffer Output buffer to be trasmited to the server
 * that don't have all informations provided by user at start
 * @param progInt Pointer to ProgramInterface
 * 
 * @return Returns true if buffer can be sended to the server
 */
bool assembleProtocol(ProtocolBlocks* pBlocks, Buffer* buffer, ProgramInterface* progInt)
{
    // calculate expected size and resize buffer accordingly, +10 is overhead for zeroing bytes, msgID, type etc...
    size_t expectedSize = pBlocks->zeroth.len + pBlocks->first.len
                        + pBlocks->second.len + pBlocks->third.len + 10;
    
    // position counter in buffer
    size_t ptrPos = 0;

    // resize buffer to needed size
    bufferResize(buffer, expectedSize);

    cmd_t type = uchar2CommandType(pBlocks->type);
    // Set correct message type to buffer, also filter commands that shouldn't be send
    switch (type)
    {
    case cmd_AUTH: buffer->data[0] = msg_AUTH; break;
    case cmd_JOIN: buffer->data[0] = msg_JOIN; 
        // add to expected size
        expectedSize += progInt->comDetails->channelID.used;
        break;
    case cmd_MSG: buffer->data[0] = msg_MSG; 
        // add to expected size
        expectedSize += progInt->comDetails->displayName.used;
        break;
    case cmd_CONF: 
        buffer->data[0] = msg_CONF;
        buffer->data[1] = pBlocks->cmd_conf_lowMsgID.start[0];
        buffer->data[2] = pBlocks->cmd_conf_highMsgID.start[0];
        buffer->used = 3;
        return true; // message can be send to server
        break;
    case cmd_EXIT: 
        buffer->data[0] = (unsigned char) msg_BYE;
        buffer->used = 3;
        return true; // message can be send to server
        break;
    default: errHandling("Unknown command type in assembleProtocol() function", 1) /*TODO: change error code*/; break;
    }

    // resize buffer to needed size, again because of join/msg
    bufferResize(buffer, expectedSize);

    ptrPos += 1;

    // Move position by number of MessageID bytes, dont set them, sender does it
    // buffer->data[1] = high;  
    ptrPos += 1;
    // buffer->data[2] = low;  
    ptrPos += 1;

    switch (type)
    {
    case cmd_AUTH:
        ADD_BLOCK_TO_BUFFER(buffer->data[ptrPos], pBlocks->cmd_auth_username);
        ADD_BLOCK_TO_BUFFER(buffer->data[ptrPos], pBlocks->cmd_auth_displayname);
        ADD_BLOCK_TO_BUFFER(buffer->data[ptrPos], pBlocks->cmd_auth_secret);
        break;
    case cmd_JOIN:
        ADD_BLOCK_TO_BUFFER(buffer->data[ptrPos], pBlocks->cmd_join_channelID);
        // check if displayname is stored
        if(progInt->comDetails->displayName.data == NULL)
        { 
            safePrintStdout("Displayname not provided, cannot join! Use /help for help.");
            return false; // message cannot be sent
        }
        
        ADD_STORED_INFO_TO_BUFFER(buffer->data[ptrPos], progInt->comDetails->displayName);
        break;
    case cmd_MSG:
        // check if displayname is stored
        if(progInt->comDetails->displayName.data == NULL)
            { 
                safePrintStdout("System: ChannelID not provided, cannot rename!"
                    "(Did you use /auth before this commands?). Use /help for help.\n");
                return false;
            }
        stringReplace(&(buffer->data[ptrPos]), progInt->comDetails->displayName.data, progInt->comDetails->displayName.used);
        ptrPos += progInt->comDetails->displayName.used;
        buffer->data[ptrPos] = 0;
        ptrPos += 1;
        // ADD_STORED_INFO_TO_BUFFER(buffer->data[ptrPos], progInt->comDetails->displayName);
        ADD_BLOCK_TO_BUFFER(buffer->data[ptrPos], pBlocks->cmd_msg_MsgContents);
        break;
    default:
        errHandling("Unknown CommandType in assembleProtocol()\n", 1); // TODO:
        break;
    }

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
 * @brief Dissassembles protocol from Buffer into commands, msgType and msgId
 * 
 * @param buffer Input buffer containing message
 * @param pBlocks Separated commands and values from user input
 * @param msgId Detected message ID 
 */
void disassebleProtocol(Buffer* buffer, ProtocolBlocks* pBlocks, uint16_t* msgId)
{
    size_t index; // Temporaty helping variable to hold index in string

    // Get msg type
    pBlocks->type = (unsigned char) buffer->data[0];
    
    *msgId = convert2BytesToU16Int(buffer->data[1], buffer->data[2]);
    
    // Set values of ProtocolBlocks
    pBlocks->zeroth.start = &(buffer->data[3]);
    pBlocks->zeroth.len = 0;

    pBlocks->first.start = NULL; pBlocks->first.len = 0;
    pBlocks->second.start = NULL; pBlocks->second.len = 0;
    pBlocks->third.start = NULL; pBlocks->third.len = 0; 
    // ------------------------------------------------------------------------

    switch (uchar2msgType(pBlocks->type))
    {
    case msg_CONF: break; // No addtional bytes needed
    case msg_REPLY:
        // Reply: Result (1 byte)
        pBlocks->msg_reply_result.start = &(buffer->data[3]);
        pBlocks->msg_reply_result.len = 1;

        // Reply: Ref_MessageID (2 bytes)
        pBlocks->msg_reply_refMsgID.start = BBLOCK_END(pBlocks->msg_reply_result);
        pBlocks->msg_reply_refMsgID.len = 2;

        // Reply: MessageContents (x bytes)
        pBlocks->msg_reply_MsgContents.start = BBLOCK_END(pBlocks->msg_reply_refMsgID);
        
        index = findZeroInString(pBlocks->msg_reply_MsgContents.start, buffer->used - 
                (TYPE_ID_LEN + pBlocks->msg_reply_result.len + pBlocks->msg_reply_refMsgID.len));
        pBlocks->msg_reply_MsgContents.len = index;
        break;
    // ------------------------------------------------------------------------
    case msg_AUTH:
        // Auth: Username (x bytes)
        index = findZeroInString(pBlocks->msg_auth_username.start, buffer->used - (TYPE_ID_LEN));
        pBlocks->msg_auth_username.len = index;

        // Auth: Secret (x bytes)
        pBlocks->msg_auth_displayname.start = BBLOCK_END_W_ZERO_BYTE(pBlocks->msg_auth_username); // skip one zero byte
        index = findZeroInString(pBlocks->msg_auth_displayname.start, buffer->used 
                - (TYPE_ID_LEN + pBlocks->msg_auth_username.len));
        pBlocks->msg_auth_displayname.len = index;

        // Auth: DisplayName (x bytes)
        pBlocks->msg_auth_secret.start = BBLOCK_END_W_ZERO_BYTE(pBlocks->msg_auth_displayname); // skip one zero byte
        index = findZeroInString(pBlocks->msg_auth_secret.start, buffer->used 
            - (TYPE_ID_LEN + pBlocks->msg_auth_username.len + pBlocks->msg_auth_displayname.len));
        pBlocks->msg_auth_secret.len = index;
        break;
    // ------------------------------------------------------------------------
    case msg_MSG:
    case msg_JOIN:
    case msg_ERR: // ERR, JOIN and MSG have same order
        // MSG, ERR: DisplayName (x bytes) / cmd_JOIN: ChannelID (x bytes) 
        index = findZeroInString(pBlocks->zeroth.start, buffer->used - (TYPE_ID_LEN));
        pBlocks->zeroth.len = index;

        // MSG, ERR: MessageContents (x bytes) / cmd_JOIN: DisplayName (x bytes) 
        pBlocks->first.start = BBLOCK_END_W_ZERO_BYTE(pBlocks->zeroth) // skip one zero byte
        index = findZeroInString(pBlocks->first.start, buffer->used 
            - (TYPE_ID_LEN + pBlocks->zeroth.len));

        pBlocks->first.len = index;
        break;
    // ------------------------------------------------------------------------
    case msg_BYE: break; // No addtional bytes needed
    default:
        debugPrint(stdout, "Unknown message type: %i\n", (int)pBlocks->type);
        errHandling("ERROR: Protocol disassembler received unknown message type", 1); // TODO:change
    }
}
#undef BBLOCK_END
#undef BBLOCK_END_W_ZERO_BYTE


// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Wrapper around getWord(), if not enough arguments given
 * change this into an plain message. 
 * 
 * This is instead of GOTO
 */
#define TRY_GET_WORD(cond) if (!(cond)) {               \
    pBlocks->cmd_msg_MsgContents.start = buffer->data;  \
    pBlocks->cmd_msg_MsgContents.len = buffer->used;    \
    pBlocks->type = cmd_MSG;                            \
    return;                                             \
    }


/**
 * @brief Takes input from user (client) from buffer and break it into an
 * array of commands (ByteBlock)
 * 
 * @warning Commands contain only pointers to the buffer, rewriting buffer
 * will lead to unexpected behavior commands in commands
 * 
 * @param buffer Input buffer containing command from user (client)
 * @param pBlocks Structure that holds user input separated into commands 
 * @param eofDetected Signals that end of file was detected
 * @param flags Flags that will be set to message
 * @return cmd_t Returns command type
 * // TODO: rewrite
 */
void userInputToCmds(Buffer* buffer, ProtocolBlocks* pBlocks, bool* eofDetected, msg_flags* flags)
{
    // If buffer is not filled skip
    if(buffer->used <= 0) { pBlocks->type = cmd_NONE; return; }

    size_t index = findBlankCharInString(buffer->data, buffer->used);
    // get command into ProtocolBlocks
    BytesBlock cmd = {.start=buffer->data, .len=index};
    pBlocks->cmd_command = cmd;

    // reset byteblocks
    BytesBlock first = {NULL, 0}, second = {NULL, 0}, third = {NULL, 0};
    pBlocks->first = first;
    pBlocks->second = second;
    pBlocks->third = third;

    int tmp = -1;

    if(*eofDetected)
    {
        pBlocks->type = cmd_EXIT;
    }
    else if(strncmp(cmd.start, "/auth", cmd.len) == 0)
    {
        // separate words and store in into BytesBlocks
        TRY_GET_WORD( getWord(&first, &(cmd.start[cmd.len]), buffer->used - (cmd.len)) )
        TRY_GET_WORD( getWord(&second, &(first.start[first.len]), buffer->used - (cmd.len + first.len)) )
        TRY_GET_WORD( getWord(&third, &(second.start[second.len]), buffer->used - (cmd.len + first.len + second.len)))

        // set message flag
        *flags = msg_flag_AUTH;

        // store information into correct ProtocolBlocks parts
        pBlocks->type = cmd_AUTH;
        pBlocks->cmd_auth_username = first;
        pBlocks->cmd_auth_secret = second;
        pBlocks->cmd_auth_displayname = third;
    }
    else if(  ((tmp = strncmp(cmd.start, "/join", cmd.len)) == 0) ||
                    (strncmp(cmd.start, "/rename", cmd.len) == 0))
    {
        // set start at after command 
        first.start = &(cmd.start[cmd.len]);
        // find first non white character
        size_t index = skipBlankCharsInString(first.start, buffer->used - cmd.len);
        // update first start position
        first.start = &(first.start[index]);
        // find end of line in string
        first.len = findNewLineInString(first.start, buffer->used - cmd.len);

        // store information into correct ProtocolBlocks parts
        if(tmp == 0)
        {
            pBlocks->type = cmd_JOIN;
            pBlocks->cmd_join_channelID = first;
        }
        else
        {
            pBlocks->type = cmd_RENAME;
            pBlocks->cmd_rename_displayname = first;     
        }
            
    }
    else if(strncmp(cmd.start, "/help", cmd.len) == 0)
    {
        // store information into correct ProtocolBlocks parts
        pBlocks->type = cmd_HELP;
    }
    else if(strncmp(cmd.start, "/exit", cmd.len) == 0)
    {
        // store information into correct ProtocolBlocks parts
        pBlocks->type = cmd_EXIT;
    }
    else
    {
        // store information into correct ProtocolBlocks parts
        pBlocks->cmd_msg_MsgContents.start = buffer->data;
        pBlocks->cmd_msg_MsgContents.len = buffer->used;
        pBlocks->type = cmd_MSG;
    }
}