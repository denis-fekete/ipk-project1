/**
 * @file ipk24protocol.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief Implmentation of functions for working with IPK24CHAT
 * Protocol such as assembling or disassembling protocols for both UDP and TCP
 * variants. Also contains separating user input into an ByteBlocks for further
 * processing.
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "ipk24protocol.h"

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Sets default values to the ProtocolBlock variable
 * 
 * @param pBlocks Pointer to the Protocol Blocks that should be resetted
 */
void resetProtocolBlocks(ProtocolBlocks* pBlocks)
{
    pBlocks->zeroth.start = NULL;    pBlocks->zeroth.len = 0;
    pBlocks->first.start = NULL;     pBlocks->first.len = 0;
    pBlocks->second.start = NULL;    pBlocks->second.len = 0;
    pBlocks->third.start = NULL;     pBlocks->third.len = 0;
}

#define ADD_BLOCK_TO_BUFFER(dst, src)           \
    stringReplace(&(dst), src.start, src.len);  \
    ptrPos += src.len;

#define ADD_ZERO_BYTE                           \
    buffer->data[ptrPos] = 0;                   \
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
    case cmd_ERR: 
        buffer->data[0] = (unsigned char) msg_ERR; 
        expectedSize += progInt->comDetails->displayName.used;
        break;
    case cmd_MSG: 
        buffer->data[0] = msg_MSG; 
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
    default: 
        errHandling("Unknown command type in assembleProtocolUDP() function", 
            err_INTERNAL_BAD_ARG);
        break;
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
        // set type of message to AUTH
        pBlocks->type = msg_AUTH;
        break;
    case cmd_JOIN:
        ADD_BLOCK_TO_BUFFER(buffer->data[ptrPos], pBlocks->cmd_join_channelID);
        ADD_ZERO_BYTE;
        // check if displayname is stored
        if(progInt->comDetails->displayName.data == NULL)
        { 
            safePrintStderr("System: Displayname not provided, cannot join! Use /help for help.\n");
            return false; // message cannot be sent
        }
        
        ADD_STORED_INFO_TO_BUFFER(buffer->data[ptrPos], progInt->comDetails->displayName);
        ADD_ZERO_BYTE;
        // set type of message to JOIN
        pBlocks->type = msg_JOIN;
        break;
    case cmd_ERR:
    case cmd_MSG:
        // check if displayname is stored
        if(progInt->comDetails->displayName.data == NULL)
        { 
            safePrintStderr("System: ChannelID not provided, cannot rename!"
                "(Did you use /auth before this commands?). Use /help for help.\n");
            return false;
        }
        // add stored displayname to the messages
        ADD_STORED_INFO_TO_BUFFER(buffer->data[ptrPos], progInt->comDetails->displayName);
        ADD_ZERO_BYTE;
        ADD_BLOCK_TO_BUFFER(buffer->data[ptrPos], pBlocks->cmd_msg_MsgContents);
        ADD_ZERO_BYTE;
        // set type of message to MSG or ERR
        pBlocks->type = (type == cmd_MSG)? msg_MSG : msg_ERR;
        break;
    default:
        errHandling("Unknown CommandType in assembleProtocolUDP()\n", 
            err_INTERNAL_BAD_ARG);
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

    // based on command increase expected size of buffer
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
        expectedSize += progInt->comDetails->channelID.used;
        break;
    case cmd_MSG:
    case cmd_ERR: // err and message are same
        // expectedSize += sizeof("MSG FROM ") -1;
        // expectedSize += sizeof("ERR FROM ") -1;
        // expectedSize += sizeof(" IS ") -1;
        expectedSize += 9 + 4; 
        expectedSize += progInt->comDetails->displayName.used;
        break;
    case cmd_EXIT:
        // expectedSize += sizeof("BYE") -1;
        expectedSize += 3;
        break;
    default: 
        errHandling("Unknown command type in assembleProtocolUDP() function", 
        err_INTERNAL_BAD_ARG);
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
                safePrintStderr("System: ChannelID not provided, cannot rename!"
                    "(Did you use /auth before this commands?). Use /help for help.\n");
                return false;
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
            safePrintStderr("System: Displayname not provided, cannot join! Use /help for help.\n");
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
        errHandling("Unknown command type in assembleProtocolUDP() function", 
            err_INTERNAL_BAD_ARG);
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
void disassebleProtocolUDP(Buffer* buffer, ProtocolBlocks* pBlocks, uint16_t* msgId)
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
        debugPrint(stdout, "a\n");
        index = findZeroInString(pBlocks->msg_auth_username.start, buffer->used - (TYPE_ID_LEN));
        pBlocks->msg_auth_username.len = index;

        debugPrint(stdout, "b\n");
        // Auth: Secret (x bytes)
        pBlocks->msg_auth_displayname.start = BBLOCK_END_W_ZERO_BYTE(pBlocks->msg_auth_username); // skip one zero byte
        index = findZeroInString(pBlocks->msg_auth_displayname.start, buffer->used 
                - (TYPE_ID_LEN + pBlocks->msg_auth_username.len));
        pBlocks->msg_auth_displayname.len = index;

        debugPrint(stdout, "c\n");
        // Auth: DisplayName (x bytes)
        pBlocks->msg_auth_secret.start = BBLOCK_END_W_ZERO_BYTE(pBlocks->msg_auth_displayname); // skip one zero byte
        index = findZeroInString(pBlocks->msg_auth_secret.start, buffer->used 
            - (TYPE_ID_LEN + pBlocks->msg_auth_username.len + pBlocks->msg_auth_displayname.len));
        pBlocks->msg_auth_secret.len = index;
        debugPrint(stdout, "d\n");
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
    case msg_BYE: // No addtional bytes needed 
        pBlocks->type = msg_BYE;
        break; 
    default:
        pBlocks->type = msg_UNKNOWN;
        return;
    }

    return;
}

#define MSG_FROM_TEXT "MSG FROM "
#define IS_TEXT " IS "
#define REPLY_TEXT "REPLY "
#define REPLY_OK_TEXT "OK IS "
#define REPLY_NOK_TEXT "NOK IS "
#define BYE_TEXT "BYE"
#define ERR_FROM_TEXT "ERR FROM "

#define LEN_OF(constString) (sizeof(constString) - 1)
/**
 * @brief Dissassembles protocol from Buffer into commands, msgType and msgId
 * for TCP variant
 * @param buffer Input buffer containing message
 * @param pBlocks Separated commands and values from user input
 * @param msgId Detected message ID 
 */
void disassebleProtocolTCP(Buffer* buffer, ProtocolBlocks* pBlocks)
{
    size_t index;

    if ( strncmp(buffer->data, MSG_FROM_TEXT, LEN_OF(MSG_FROM_TEXT)) == 0 )
    {
        pBlocks->msg_msg_displayname.start = &(buffer->data[LEN_OF(MSG_FROM_TEXT)]);
        // find end of displayname after sizeof("MSG FROM ") =  len: 9 (\0 not counted)
        index = findBlankCharInString(pBlocks->msg_msg_displayname.start, buffer->used - LEN_OF(MSG_FROM_TEXT));
        pBlocks->msg_msg_displayname.len = index;
        
        // check if there is " IS " correctly typed after {displayname}
        if(strncmp(BBLOCK_END(pBlocks->msg_msg_displayname), IS_TEXT, LEN_OF(IS_TEXT)) != 0)
        {
            pBlocks->type = msg_CORRUPTED;
            return;
        }

        // set start of the message contets after displayname and add " IS " to it
        pBlocks->msg_msg_MsgContents.start = 
            &(pBlocks->msg_msg_displayname.start[pBlocks->msg_msg_displayname.len + 
            LEN_OF(IS_TEXT)]);

        // find end of message contents
        index = findNewLineInString(pBlocks->msg_msg_MsgContents.start, 
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
        else { pBlocks->type = msg_CORRUPTED; return; }

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
        pBlocks->msg_err_displayname.start = &(buffer->data[LEN_OF(MSG_FROM_TEXT)]);
        // find end of displayname after sizeof("MSG FROM ") =  len: 9 (\0 not counted)
        index = findBlankCharInString(pBlocks->msg_err_displayname.start, buffer->used - LEN_OF(MSG_FROM_TEXT));
        pBlocks->msg_err_displayname.len = index;
        
        // check if there is " IS " correctly typed after {displayname}
        if(strncmp(BBLOCK_END(pBlocks->msg_err_displayname), IS_TEXT, LEN_OF(IS_TEXT)) != 0)
        {
            pBlocks->type = msg_CORRUPTED;
            return;
        }

        // set start of the message contets after displayname and add " IS " to it
        pBlocks->msg_err_MsgContents.start = 
            &(pBlocks->msg_err_displayname.start[pBlocks->msg_err_displayname.len + 
            LEN_OF(IS_TEXT)]);

        // find end of message contents
        index = findNewLineInString(pBlocks->msg_err_MsgContents.start, 
            buffer->used - LEN_OF(MSG_FROM_TEXT) - pBlocks->msg_err_displayname.len);
            
        pBlocks->msg_err_MsgContents.len = index;
        pBlocks->type = msg_ERR;
    }
    else
    {
        pBlocks->type = msg_UNKNOWN;
        return;
    }

    return;
}

/**
 * @brief Checks if input character is alligable to be in credentials 
 * 
 * @param input Input character
 * @return true Character is alligable
 * @return false Character is not alligable
 */
bool allowedCharsInCredentials(char input)
{
    if(input >= 'a' && input <= 'z')
        return true;
    else if(input >= 'A' && input <= 'Z')
        return true;
    else if(input >= '0' && input <= '9')
        return true;
    else if(input == '-')
        return true;
    #ifdef DEBUG // allow "." in debug mode
        if(input == '.')
        {
            return true;
        }
    #endif

    return false;
}

/**
 * @brief Checks if input character is alligable to be in display name 
 * 
 * @param input Input character
 * @return true Character is alligable
 * @return false Character is not alligable
 */
bool allowedCharsInName(char input)
{
    if(input >= 0x21 && input <= 0x7E)
        return true;

    return false;
}  

/**
 * @brief Checks if input character is alligable to be in message contents 
 * 
 * @param input Input character
 * @return true Character is alligable
 * @return false Character is not alligable
 */
bool allowedCharsInMessage(char input)
{
    if(input >= 0x20 && input <= 0x7E)
        return true;
        
    return false;
}  

// Types of Words
#define ToW_Username 1
#define ToW_ChannelID 2
#define ToW_Secret 3
#define ToW_DisplayName 4
#define ToW_MessageContent 5
/**
 * @brief Controls whenever word is valid
 * 
 * @param wordBlock Pointer to be ByteBlock that holds word values
 * @param maxLen Maximum allowed length for this word
 * @param typeOfWord Type of Word, characters will controlled by this argument
 * @return true Word is valid
 * @return false Word is not valid
 */
bool controlWord(BytesBlock* wordBlock, const size_t maxLen, unsigned char typeOfWord)
{
    if(wordBlock->len <= 0)
    {
        return false;
    }
    // check if word has correct length
    if(wordBlock->len > maxLen)
    {
        fprintf(stderr, "ERR: Parameter \"");
        for(size_t i = 0; i < wordBlock->len; i++)
        {
            fprintf(stderr, "%c", wordBlock->start[i]);
        }
        fprintf(stderr, "\" exceeded maximum allowed length (%li). "
            "Message will not be sended.\n", maxLen);

        return false;
    }
    
    // check all characters in word
    char correct = true;
    size_t i = 0;
    for(; i < wordBlock->len; i++)
    {
        if( typeOfWord == ToW_Username || typeOfWord == ToW_ChannelID ||
            typeOfWord == ToW_Secret)
        {
            if( !allowedCharsInCredentials(wordBlock->start[i]))
            {
                correct = false;
                break;
            }
        }
        else if(typeOfWord == ToW_DisplayName)
        {
            if( !allowedCharsInName(wordBlock->start[i]) )
            {
                correct = false;
                break;
            }
        }
        else if(typeOfWord == ToW_MessageContent)
        {
            if( !allowedCharsInMessage(wordBlock->start[i]) )
            {
                correct = false;
                break;
            }   
        }
    }

    // if "i" didn't control whole word
    if(!correct)
    {
        fprintf(stderr, "ERR: Parameter \"");
        for(size_t i = 0; i < wordBlock->len; i++)
        {
            fprintf(stderr, "%c", wordBlock->start[i]);
        }
        fprintf(stderr, "\" contains invalid character \"%c\" at position: %li. "
            "Message will not be sended.\n", wordBlock->start[i], i);

        return false;
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
 * change all other commands into an message. 
 * 
 * This is instead of GOTO
 */
#define TRY_GET_WORD(cond) if (!(cond)) {           \
        pBlocks->type = cmd_MISSING;                \
        return false;                               \
    }

/**
 * @brief Sets correct block type and exits function with false 
 */
#define IF_ERR_SET_CMDTYPE(cmdT) if(error == -1) { pBlocks->type = cmdT; return false; }

/**
 * @brief Takes input from user (client) from buffer and break it into an
 * array of commands (ByteBlock)
 * 
 * @warning Commands contain only pointers to the buffer, rewriting buffer
 * will lead to unexpected behavior commands in commands
 * 
 * @param buffer Input buffer containing command from user (client)
 * @param commands Array of commands where separated commands will be store 
 * @param flags Flags that will be set to message
 * @return bool Returns whenever parameters are valid
 */
int userInputToCmds(Buffer* buffer, ProtocolBlocks* pBlocks, msg_flags* flags)
{
    // If buffer is not filled skip
    if(buffer->used <= 0) { pBlocks->type = cmd_NONE; return false; }

    size_t index = findBlankCharInString(buffer->data, buffer->used);
    // get command into ProtocolBlocks
    BytesBlock cmd = {.start=buffer->data, .len=index};
    pBlocks->cmd_command = cmd;
    
    // reset byteblocks
    BytesBlock first = {NULL, 0}, second = {NULL, 0}, third = {NULL, 0};
    pBlocks->first = first;
    pBlocks->second = second;
    pBlocks->third = third;
    pBlocks->type = cmd_NONE;

    long error = 0;

    int tmp = -1;

    if(strncmp(cmd.start, "/auth", cmd.len) == 0)
    {
        // separate words and store in into BytesBlocks
        TRY_GET_WORD( getWord(&first, &(cmd.start[cmd.len]), buffer->used - (cmd.len)) )
        TRY_GET_WORD( getWord(&second, &(first.start[first.len]), buffer->used - (cmd.len + first.len)) )
        TRY_GET_WORD( getWord(&third, &(second.start[second.len]), buffer->used - (cmd.len + first.len + second.len)))

        if(!controlWord(&first, 20, ToW_Username)) { return false; }
        if(!controlWord(&second, 128, ToW_Secret)) { return false; }
        if(!controlWord(&third, 20, ToW_DisplayName)) { return false; }

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
        error = findNewLineInString(first.start, buffer->used - cmd.len);

        IF_ERR_SET_CMDTYPE(cmd_MISSING);

        first.len = (size_t) error;

        // store information into correct ProtocolBlocks parts
        // if tmp == 0 join was provided
        if(tmp == 0)
        {
            if(!controlWord(&first, 20, ToW_ChannelID)) { return false; }
            pBlocks->type = cmd_JOIN;
            pBlocks->cmd_join_channelID = first;
        }
        else
        {
            if(!controlWord(&first, 20, ToW_DisplayName)) { return false; }
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
        if(!controlWord(&pBlocks->cmd_msg_MsgContents, 14000, ToW_MessageContent)) { return false; }

        pBlocks->type = cmd_MSG;
    }

    return true;
}

#undef ToW_Username
#undef ToW_ChannelID
#undef ToW_Secret
#undef ToW_DisplayName
#undef ToW_MessageContent