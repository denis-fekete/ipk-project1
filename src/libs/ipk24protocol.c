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
    ptrPos += src.len;

#define ADD_ZERO_BYTE           \
    buffer->data[ptrPos] = 0;   \
    ptrPos += 1;

#define ADD_STORED_INFO_TO_BUFFER(dst, src)     \
    stringReplace(&(dst), src.data, src.used);  \
    ptrPos += src.used;                         \

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
bool assembleProtocolUDP(ProtocolBlocks* pBlocks, Buffer* buffer, ProgramInterface* progInt)
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
    default: errHandling("Unknown command type in assembleProtocolUDP() function", 1) /*TODO: change error code*/; break;
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
        ADD_ZERO_BYTE;
        ADD_BLOCK_TO_BUFFER(buffer->data[ptrPos], pBlocks->cmd_auth_displayname);
        ADD_ZERO_BYTE;
        ADD_BLOCK_TO_BUFFER(buffer->data[ptrPos], pBlocks->cmd_auth_secret);
        ADD_ZERO_BYTE;
        break;
    case cmd_JOIN:
        ADD_BLOCK_TO_BUFFER(buffer->data[ptrPos], pBlocks->cmd_join_channelID);
        ADD_ZERO_BYTE;
        // check if displayname is stored
        if(progInt->comDetails->displayName.data == NULL)
        { 
            safePrintStdout("System: Displayname not provided, cannot join! Use /help for help.\n");
            return false; // message cannot be sent
        }
        
        ADD_STORED_INFO_TO_BUFFER(buffer->data[ptrPos], progInt->comDetails->displayName);
        ADD_ZERO_BYTE;
        break;
    case cmd_MSG:
    case cmd_ERR: // err is same as msg
        // check if displayname is stored
        if(progInt->comDetails->displayName.data == NULL)
            { 
                safePrintStdout("System: ChannelID not provided, cannot rename!"
                    "(Did you use /auth before this commands?). Use /help for help.\n");
                return false;
            }
        stringReplace(&(buffer->data[ptrPos]), progInt->comDetails->displayName.data, progInt->comDetails->displayName.used);
        ptrPos += progInt->comDetails->displayName.used;
        ADD_ZERO_BYTE;
        // ADD_STORED_INFO_TO_BUFFER(buffer->data[ptrPos], progInt->comDetails->displayName);
        ADD_BLOCK_TO_BUFFER(buffer->data[ptrPos], pBlocks->cmd_msg_MsgContents);
        ADD_ZERO_BYTE;
        break;
    default:
        errHandling("Unknown CommandType in assembleProtocolUDP()\n", 1); // TODO:
        break;
    }

    buffer->used = ptrPos;

    return true;
}
#undef BLOCK_TO_BUFF

#define ADD_STRING_TO_BUFFER(dst, string)   \
    stringReplace(&(dst), string, sizeof(string) - 1); /*-1 to not include \0*/ \
    ptrPos += sizeof(string) - 1;

/**
 * @brief Assembles protocol from commands and command type into a buffer in
 * TCP format
 * 
 * @param pBlocks Separated commands and values from user input
 * @param buffer Output buffer to be trasmited to the server
 * that don't have all informations provided by user at start
 * @param progInt Pointer to ProgramInterface
 * 
 * @return Returns true if buffer can be sended to the server
 */
bool assembleProtocolTCP(ProtocolBlocks* pBlocks, Buffer* buffer, ProgramInterface* progInt)
{
    // calculate expected size and resize buffer accordingly, +1 zero byte.
    size_t expectedSize = pBlocks->zeroth.len + pBlocks->first.len
                        + pBlocks->second.len + pBlocks->third.len + 1;

    // resize buffer to needed size
    bufferResize(buffer, expectedSize);

    switch (uchar2CommandType(pBlocks->type))
    {
    case cmd_AUTH:
        // expectedSize += sizeof("AUTH ") - 1;
        // expectedSize += sizeof(" AS ") - 1;
        // expectedSize += sizeof(" USING ") -1;
        expectedSize += 5 + 4 + 7;
        break;
    case cmd_JOIN:
        // expectedSize += sizeof("JOIN ") -1;
        // expectedSize += sizeof(" AS ") -1;
        expectedSize += 5 + 4;
        break;
    case cmd_MSG:
    case cmd_ERR: // err and message are same
        // expectedSize += sizeof("MSG FROM ") -1;
        // expectedSize += sizeof("ERR FROM ") -1;
        // expectedSize += sizeof(" IS ") -1;
        expectedSize += 9 + 4; 
        break;
    case cmd_EXIT:
        // expectedSize += sizeof("BYE") -1;
        expectedSize += 3;
        break;
    default: 
        errHandling("Unknown command type in assembleProtocolUDP() function", 1); /*TODO: change error code*/
        break;
    }
    // expectedSize += sizeof('\r') + sizeof('\n') -2;
    expectedSize += 2; // "\r\n"

    // resize buffer to needed size
    bufferResize(buffer, expectedSize);
    size_t ptrPos = 0;

    switch (uchar2CommandType(pBlocks->type))
    {
    case cmd_AUTH:
        ADD_STRING_TO_BUFFER(buffer->data[0], "AUTH ");
        ADD_BLOCK_TO_BUFFER(buffer->data[ptrPos], pBlocks->cmd_auth_username);
        ADD_STRING_TO_BUFFER(buffer->data[ptrPos], " AS ");
        ADD_BLOCK_TO_BUFFER(buffer->data[ptrPos], pBlocks->cmd_auth_displayname);
        ADD_STRING_TO_BUFFER(buffer->data[ptrPos], " USING ");
        ADD_BLOCK_TO_BUFFER(buffer->data[ptrPos], pBlocks->cmd_auth_secret);

        pBlocks->type = msg_AUTH;
        break;
    case cmd_MSG:
    case cmd_ERR: // err and msg are same
        // check if displayname is stored
        if(progInt->comDetails->displayName.data == NULL)
            { 
                safePrintStdout("System: ChannelID not provided, cannot rename!"
                    "(Did you use /auth before this commands?). Use /help for help.\n");
                return false; // TODO: check what  to do in this state
        }

        if(uchar2CommandType(pBlocks->type) == cmd_MSG)
        {
            ADD_STRING_TO_BUFFER(buffer->data[0], "MSG FROM ");
            pBlocks->type = msg_MSG;
        }
        else
        { 
            ADD_STRING_TO_BUFFER(buffer->data[0], "ERR FROM ");
            pBlocks->type = msg_ERR;
        }

        ADD_STORED_INFO_TO_BUFFER(buffer->data[ptrPos], progInt->comDetails->displayName);
        ADD_STRING_TO_BUFFER(buffer->data[ptrPos], " IS ");
        ADD_BLOCK_TO_BUFFER(buffer->data[ptrPos], pBlocks->cmd_msg_MsgContents);

        break;
    case cmd_JOIN:
        // check if displayname is stored
        if(progInt->comDetails->displayName.data == NULL)
        { 
            safePrintStdout("System: Displayname not provided, cannot join! Use /help for help.\n");
            return false; // message cannot be sent
        }

        ADD_STRING_TO_BUFFER(buffer->data[0], "JOIN ");
        ADD_STORED_INFO_TO_BUFFER(buffer->data[ptrPos], progInt->comDetails->channelID);
        ADD_STRING_TO_BUFFER(buffer->data[ptrPos], " AS ");
        ADD_STORED_INFO_TO_BUFFER(buffer->data[ptrPos], progInt->comDetails->displayName);

        pBlocks->type = msg_JOIN;
        break;
    case cmd_EXIT:
        ADD_STRING_TO_BUFFER(buffer->data[0], "BYE");
        pBlocks->type = msg_BYE;
        break;
    default: 
        errHandling("Unknown command type in assembleProtocolUDP() function", 1); /*TODO: change error code*/
        break;
    }
    // \r and \n at the od of string, also add zerobyte
    ADD_STRING_TO_BUFFER(buffer->data[ptrPos], "\r\n");

    buffer->used = ptrPos;

    return true;
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------


#define BBLOCK_END(block) &(block.start[block.len])
#define BBLOCK_END_W_ZERO_BYTE(block) &(block.start[block.len + 1])
#define TYPE_ID_LEN 3

/**
 * @brief Dissassembles protocol from Buffer into commands, msgType and msgId
 * for UDP variant
 * @param buffer Input buffer containing message
 * @param pBlocks Separated commands and values from user input
 * @param msgId Detected message ID 
 */
bool disassebleProtocolUDP(Buffer* buffer, ProtocolBlocks* pBlocks, uint16_t* msgId)
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
        pBlocks->first.start = BBLOCK_END_W_ZERO_BYTE(pBlocks->zeroth); // skip one zero byte
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

    return true;
}

#define MSG_FROM_TEXT "MSG FROM "
#define IS_TEXT " IS "
#define REPLY_TEXT "REPLY "
#define REPLY_OK_TEXT "OK IS "
#define REPLY_NOK_TEXT "NOK IS "
#define BYE_TEXT "BYE"
#define ERR_FROM_TEXT "ERR FROM"

#define LEN_OF(constString) (sizeof(constString) - 1)
/**
 * @brief Dissassembles protocol from Buffer into commands, msgType and msgId
 * for TCP variant
 * @param buffer Input buffer containing message
 * @param pBlocks Separated commands and values from user input
 * @param msgId Detected message ID 
 */
bool disassebleProtocolTCP(Buffer* buffer, ProtocolBlocks* pBlocks)
{
    size_t index;

    if ( strncmp(buffer->data, MSG_FROM_TEXT, LEN_OF(MSG_FROM_TEXT)) == 0 )
    {
        // find end of displayname after sizeof("MSG FROM ") =  len: 9 (\0 not counted)
        pBlocks->msg_msg_displayname.start = &(buffer->data[LEN_OF(MSG_FROM_TEXT)]);
        index = findBlankCharInString(pBlocks->msg_msg_displayname.start, buffer->used - LEN_OF(MSG_FROM_TEXT));
        pBlocks->msg_msg_displayname.len = index;
        
        // check if there is " IS " correctly typed after {displayname}
        if(strncmp(BBLOCK_END(pBlocks->msg_msg_displayname), IS_TEXT, LEN_OF(IS_TEXT)) != 0)
        {
            return false;
        }

        // find end of message contents
        pBlocks->msg_msg_MsgContents.start = BBLOCK_END(pBlocks->msg_msg_displayname);
        index = findBlankCharInString(pBlocks->msg_msg_MsgContents.start, 
            buffer->used - LEN_OF(MSG_FROM_TEXT) - pBlocks->msg_msg_displayname.len);
            
        pBlocks->msg_msg_MsgContents.len = index;
        pBlocks->type = msg_MSG;
    }
    else if ( strncmp(buffer->data, REPLY_TEXT, LEN_OF(REPLY_TEXT)) == 0 )
    {
        // store result of reply
        pBlocks->msg_reply_result.start = &(buffer->data[LEN_OF(REPLY_TEXT)]); 
        if(strncmp(pBlocks->msg_reply_result.start,
            REPLY_OK_TEXT, LEN_OF(REPLY_OK_TEXT)) == 0)
        {
            pBlocks->msg_reply_result.len = 2;
            pBlocks->msg_reply_result_bool = true; 
        } 
        else if(strncmp(&(buffer->data[LEN_OF(REPLY_TEXT)]), 
            REPLY_NOK_TEXT, LEN_OF(REPLY_NOK_TEXT)) == 0)
        {
            pBlocks->msg_reply_result.len = 3;
            pBlocks->msg_reply_result_bool = false; 

        }
        else { return false; }

        // find end of message contents after sizeof("{RESULT} IS "), + sizeof(" IS ") = len: 4 (\0 not counted)
        pBlocks->msg_reply_MsgContents.start = 
            &(pBlocks->msg_reply_result.start
                [pBlocks->msg_reply_result.len + LEN_OF(IS_TEXT)]);

        index = findBlankCharInString(pBlocks->msg_reply_MsgContents.start, buffer->used - 
            (LEN_OF(REPLY_TEXT) + pBlocks->msg_reply_result.len) );

        // set start of msgContents right after "{RESULT} IS "
        pBlocks->msg_reply_MsgContents.len = index;
        pBlocks->type = msg_REPLY;
    }
    else if ( strncmp(buffer->data, BYE_TEXT, LEN_OF(BYE_TEXT)) == 0 )
    {
        pBlocks->type = msg_BYE;
    }
    else if ( strncmp(buffer->data, ERR_FROM_TEXT, LEN_OF(ERR_FROM_TEXT)) == 0 )
    {
        // find end of displayname after sizeof("MSG FROM ") =  len: 9 (\0 not counted)
        pBlocks->msg_err_displayname.start = &(buffer->data[LEN_OF(MSG_FROM_TEXT)]);
        index = findBlankCharInString(pBlocks->msg_err_displayname.start, buffer->used - LEN_OF(MSG_FROM_TEXT));
        pBlocks->msg_err_displayname.len = index;
        
        // check if there is " IS " correctly typed after {displayname}
        if(strncmp(BBLOCK_END(pBlocks->msg_err_displayname), IS_TEXT, LEN_OF(IS_TEXT)) != 0)
        {
            return false;
        }

        // find end of message contents
        pBlocks->msg_err_MsgContents.start = BBLOCK_END(pBlocks->msg_err_displayname);
        index = findBlankCharInString(pBlocks->msg_err_MsgContents.start, 
            buffer->used - LEN_OF(MSG_FROM_TEXT) - pBlocks->msg_err_displayname.len);
            
        pBlocks->msg_err_MsgContents.len = index;
        pBlocks->type = msg_MSG;
    }
    else
    {
        errHandling("Unknown received message", 1); //TODO:
    }

    return true;
}

#undef MSG_FROM_TEXT
#undef IS_TEXT
#undef REPLY_TEXT
#undef REPLY_OK_TEXT
#undef REPLY_NOK_TEXT
#undef BYE_TEXT
#undef ERR_FROM_TEXT
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