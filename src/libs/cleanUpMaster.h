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

#endif /*CLEAN_UP_MASTER_H*/