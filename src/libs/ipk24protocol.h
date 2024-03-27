/**
 * @file ipk24protocol.h
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef IPK24_PROTOCOL_H
#define IPK24_PROTOCOL_H

#include "msgQueue.h"

/**
 * @brief Sturcture for assembling and dissasembling protocols to make 
 * more readable what is getting store where with the use of unions.
 * 
 */
typedef struct ProtocolBlocks {
    unsigned char type;
    uint16_t msgID;

    // 0th block
    union 
    {
        BytesBlock zeroth;
        BytesBlock cmd_command;
        BytesBlock cmd_msg_MsgContents;
        BytesBlock cmd_conf_lowMsgID;

        BytesBlock msg_reply_result;
        BytesBlock msg_auth_username;
        BytesBlock msg_join_channelID;
        BytesBlock msg_msg_displayname;
        BytesBlock msg_err_displayname;
    };
    // 1st block
    union
    {
        BytesBlock first;
        BytesBlock cmd_auth_username;
        BytesBlock cmd_join_channelID;
        BytesBlock cmd_rename_displayname;
        BytesBlock cmd_conf_highMsgID;

        BytesBlock msg_reply_refMsgID;
        BytesBlock msg_auth_displayname;
        BytesBlock msg_join_displayname;
        BytesBlock msg_msg_MsgContents;
        BytesBlock msg_err_MsgContents;
    };
    // 2nd block
    union
    {
        BytesBlock second;
        BytesBlock cmd_auth_secret;

        BytesBlock msg_reply_MsgContents;
        BytesBlock msg_auth_secret;
    };
    // 3rd block
    union
    {
        BytesBlock third;
        BytesBlock cmd_auth_displayname;
    };
} ProtocolBlocks;

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
bool assembleProtocol(ProtocolBlocks* pBlocks, Buffer* buffer, ProgramInterface* progInt);

/**
 * @brief Dissassembles protocol from Buffer into commands, msgType and msgId
 * 
 * @param buffer Input buffer containing message
 * @param pBlocks Separated commands and values from user input
 * @param msgId Detected message ID 
 */
void disassebleProtocol(Buffer* buffer, ProtocolBlocks* pBlocks, uint16_t* msgId);

#include "msgQueue.h"

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
void userInputToCmds(Buffer* buffer, ProtocolBlocks* pBlocks, bool* eofDetected, msg_flags* flags);


#endif