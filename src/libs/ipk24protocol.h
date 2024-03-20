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
bool assembleProtocol(cmd_t type, BytesBlock commands[4], Buffer* buffer, ProgramInterface* progInt);

/**
 * @brief Disassebles protocol from provided Buffer to commands, 
 * message type and message ID
 * 
 * @param buffer Input buffer which will be disassembled 
 * @param commands Output commands which will be filled
 * @param msgType Message type that was detected
 * @param msgId Message ID that was detected
 */
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
cmd_t userInputToCmds(Buffer* buffer, BytesBlock commands[4], bool* eofDetected);

/**
 * @brief Converts 16bit message id into an two unsinged chars
 * 
 * @param high Output pointer to unsigned char, upper/higher half of number
 * @param low Output pointer to unsigned char, lower half of number
 * @param msgCounter Input number to be separated
 */
void breakU16IntToBytes(char* high, char* low, uint16_t msgCounter);

/**
 * @brief Converts bwo bytes from input char array into 16bit usigned integer
 * 
 * @param high Higher byte
 * @param low Lower byte
 * @return u_int16_t 
 */
u_int16_t convert2BytesToU16Int(char high, char low);
#endif