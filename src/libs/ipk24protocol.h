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

#include "customtypes.h"
#include "buffer.h"
#include "utils.h"

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
bool assembleProtocol(cmd_t type, BytesBlock commands[4], Buffer* buffer, CommunicationDetails* comDetails);

void disassebleProtocol(Buffer* buffer, BytesBlock commands[4], msg_t* msgType, u_int16_t* msgId);

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
cmd_t userInputToCmds(Buffer* buffer, BytesBlock commands[4]);
#endif