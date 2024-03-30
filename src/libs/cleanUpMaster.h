/**
 * @file cleanUpMaster.h
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief Declarations of functions for Program Interface 
 * initializing and destroying, as well as function for handling SIGINT
 * singals.
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef CLEAN_UP_MASTER_H
#define CLEAN_UP_MASTER_H

#include "../protocolReceiver.h"
#include "../protocolSender.h"

/**
 * @brief Initializes allocated structures in program interface
 * 
 */
void programInterfaceInit(ProgramInterface* pI);

/**
 * @brief Destorys and frees allocated structures inside global program
 * interface
 * 
 */
void programInterfaceDestroy(ProgramInterface* pI);

/**
 * @brief Function that handles SIGINT signals and correctly
 * frees up all memory and closes add comminication 
 * 
 * @param num 
 */
void sigintHandler(int num);

#endif /*CLEAN_UP_MASTER_H*/