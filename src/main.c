/**
 * @file main.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

// #include "netinet/ether.h"
// #include "netinet/tcp.h"
// #include "netinet/udp.h"
// #include "netinet/ip_icmp.h"

#include "getopt.h" // argument processing

// #include "libs/customtypes.h"
// #include "libs/buffer.h"
// #include "libs/utils.h"
// #include "libs/networkCom.h"
// #include "libs/msgQueue.h"
// #include "libs/ipk24protocol.h"

#include "protocolReceiver.h"
#include "protocolSender.h"

#ifdef DEBUG
    pthread_mutex_t debugPrintMutex = PTHREAD_MUTEX_INITIALIZER;
#endif

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

//Allow change of executable name in Makefile using -D{EXECUTABLE_NAME}
#ifndef EXECUTABLE_NAME
    #define EXECUTABLE_NAME "ipk24chat"
#endif

/**
 * @brief Processes arguments provided by user
 * 
 * @param argc Number of arguments given
 * @param argv Array of arguments strings (char pointers)
 */
void processArguments(int argc, char* argv[], enum Protocols* prot, Buffer* ipAddress, uint16_t* portNum, uint16_t* udpTimeout, uint8_t* udpRetrans)
{
    int opt;
    size_t optLen;
    while((opt = getopt(argc, argv, "ht:s:p:d:r:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            printCliHelpMenu(EXECUTABLE_NAME);
            exit(EXIT_SUCCESS);
            break;
        case 't':
            if(strcmp(optarg, "udp") == 0) { *prot = prot_UDP; }
            else if(strcmp(optarg, "tcp") == 0) { *prot = prot_TCP; }
            else
            {
                // TODO: change err code
                errHandling("Unknown protocol provided in -t option. Use -h for help", 1);
            }
            break;
        case 's': ; // compiler doesn't like size_t being after : 
            optLen = strlen(optarg);
            bufferResize(ipAddress, optLen + 1);
            stringReplace(ipAddress->data, optarg, optLen);
            ipAddress->data[optLen] = '\0';
            ipAddress->used = optLen + 1;
            break;
        case 'p':
            *portNum = (uint16_t)atoi(optarg);
            break;
        case 'd':
            *udpTimeout = (uint16_t)atoi(optarg);
            break;
        case 'r':
            *udpRetrans = (uint8_t)atoi(optarg);
            break;
        default:
            errHandling("Unknown option. Use -h for help", 1); //TODO: change err code
            break;
        }
    }
}


// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Updates values in CommunicationDetails comDetails variable
 * based on CommandType provided in ProgramBlocks.  
 * 
 * @note Stored data is copied, so commands can be safely rewritten
 * 
 * @param pBlocks Structure that holds separated commands and type
 * @param comDetails Pointer to the CommunicationDetails variable that will 
 * be updated
 */
void storeInformation(ProtocolBlocks* pBlocks, CommunicationDetails* comDetails)
{
    
    switch (uchar2CommandType(pBlocks->type))
    {
    case cmd_AUTH:
        // commands: CMD, USERNAME, SECRET, DISPLAYNAME
        bufferResize(&(comDetails->displayName), pBlocks->cmd_auth_displayname.len + 1);

        stringReplace(  comDetails->displayName.data, 
                        pBlocks->cmd_auth_displayname.start, 
                        pBlocks->cmd_auth_displayname.len);

        comDetails->displayName.data[pBlocks->cmd_auth_displayname.len] = '\0';
        comDetails->displayName.used = pBlocks->cmd_auth_displayname.len;
        break;
    case cmd_JOIN:
        // commands: CMD, CHANNELID
        bufferResize(&(comDetails->channelID), pBlocks->cmd_join_channelID.len + 1);

        stringReplace(  comDetails->channelID.data, 
                        pBlocks->cmd_join_channelID.start, 
                        pBlocks->cmd_join_channelID.len);
        comDetails->channelID.used = pBlocks->cmd_join_channelID.len;
        break;
    case cmd_RENAME:
        // commands: CMD, DISPLAYNAME
        bufferResize(&(comDetails->displayName), pBlocks->cmd_rename_displayname.len + 1);
        stringReplace(  comDetails->displayName.data, 
                        pBlocks->cmd_rename_displayname.start, 
                        pBlocks->cmd_rename_displayname.len);
        comDetails->displayName.data[pBlocks->cmd_rename_displayname.len] = '\0';
        comDetails->displayName.used = pBlocks->cmd_rename_displayname.len;
        break;
    default: break;
    }
}


/**
 * @brief Filters commands by CommandType (cmd_t) and returns 
 * if they should be sended
 * 
 * @param cmdType Detected command type 
 * @param progInt Pointer to the ProgramInterface
 * @param flags Flags to be set to the message
 * @return true Message can be sended
 * @return false Message cannot be sended
 */
bool filterCommandsByFSM(ProtocolBlocks* pBlocks, ProgramInterface* progInt, msg_flags* flags)
{
    // if buffer is empty ... newline was entered
    if(uchar2CommandType(pBlocks->type) == cmd_NONE) return false;

    switch (uchar2CommandType(pBlocks->type))
    {
    case cmd_AUTH:
        if( getProgramState(progInt) == fsm_START )
        {
            *flags = msg_flag_AUTH;
            setProgramState(progInt, fsm_AUTH_W82_BE_SENDED);
            return true;
        }
        break;
    case cmd_HELP:
        printUserHelpMenu(progInt);
        return false;
    case cmd_RENAME:
        // replace displayname stored in Communication Details with data 
        // from user provided command
        bufferResize(   &(progInt->comDetails->displayName), 
                        pBlocks->cmd_rename_displayname.len + 1);
        stringReplace(  progInt->comDetails->displayName.data, 
                        pBlocks->cmd_rename_displayname.start, 
                        pBlocks->cmd_rename_displayname.len);
        progInt->comDetails->displayName.used = pBlocks->cmd_rename_displayname.len;
        progInt->comDetails->displayName.data[progInt->comDetails->displayName.used] = 0;
        return false;
    default:
        if( getProgramState(progInt) != fsm_OPEN)
        {
            safePrintStdout("System: You are not connected to server! "
                "Use /auth to connect to server or /help for more information.\n"); 
            return false;
        }
        return true;
        break;
    }

    // should this message be not send?
    return true;
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

void userCommandHandling(ProgramInterface* progInt, Buffer* clientInput)
{
    Buffer protocolMsg;
    bufferInit(&protocolMsg);

    bool canBeSended;
    bool eofDetected = false;
    msg_flags flags = msg_flag_NONE;
    ProtocolBlocks pBlocks;

    // continue while continueProgram is true, only main id can go into this loop
    while (getProgramState(progInt) != fsm_END)
    {
        // --------------------------------------------------------------------
        // Convert user input into an protocol
        // --------------------------------------------------------------------
        canBeSended = false;
        eofDetected = false;
        // Load buffer from stdin, store length of buffer
        clientInput->used = loadBufferFromStdin(clientInput, &eofDetected);
        // Separate clientCommands buffer into commands (ByteBlocks),
        // store recognized command
        flags = msg_flag_NONE;
        userInputToCmds(clientInput, &pBlocks, &eofDetected, &flags);

        // Filter commands by type and FSM state
        canBeSended = filterCommandsByFSM(&pBlocks, progInt, &flags);
        // if message should not be send skip it
        if(!canBeSended) { continue; }
        // update stored information about communication 
        storeInformation(&pBlocks, progInt->comDetails);
        
        // Assembles array of bytes into Buffer protocolMsg, returns if 
        // message can be trasmitted
        canBeSended = assembleProtocol(&pBlocks, &protocolMsg, progInt);

        if(canBeSended)
        {
            // add message to the queue
            queueLock(progInt->threads->sendingQueue);

            bool signalSender = false;
            if(queueIsEmpty(progInt->threads->sendingQueue)) { signalSender = true; }
            
            queueAddMessage(progInt->threads->sendingQueue, &protocolMsg, flags);
            
            // signal sender if he is waiting because queue is empty
            if(signalSender)
            {
                pthread_cond_signal(progInt->threads->senderEmptyQueueCond);
            }
            queueUnlock(progInt->threads->sendingQueue);
        }
        
        // Exit loop if /exit detected 
        if(pBlocks.type == cmd_EXIT)
        {
            // set state to empty queue, send bye and exit
            setProgramState(progInt, fsm_EMPTY_Q_BYE);
            // wake up sender to exit
            pthread_cond_signal(progInt->threads->senderEmptyQueueCond);
        }
    }

    // free allocated memory
    free(protocolMsg.data);
}


// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------



int main(int argc, char* argv[])
{
    // ------------------------------------------------------------------------
    // Initialize interface for program
    // ------------------------------------------------------------------------

    ProgramInterface progInterface;
    ProgramInterface* progInt = &progInterface; // this is done so safePrint macro works

    ThreadCommunication threads;
    progInt->threads = &threads; 

    progInt->threads->continueProgram = true;
    progInt->threads->fsmState = fsm_START;

    MessageQueue sendingQueue;
    queueInit(&sendingQueue); // queue of outcoming (user sent) messages

    progInt->threads->sendingQueue = &sendingQueue;
    
    pthread_cond_t senderEmptyQueueCond;
    pthread_cond_init(&senderEmptyQueueCond, NULL);
    pthread_mutex_t senderEmptyQueueMutex;
    pthread_mutex_init(&senderEmptyQueueMutex, NULL);

    pthread_cond_t rec2SenderCond;
    pthread_cond_init(&rec2SenderCond, NULL);
    pthread_mutex_t rec2SenderMutex;
    pthread_mutex_init(&rec2SenderMutex, NULL);

    pthread_cond_t mainCond;
    pthread_cond_init(&mainCond, NULL);
    pthread_mutex_t mainMutex;
    pthread_mutex_init(&mainMutex, NULL);

    pthread_mutex_t stdoutMutex;
    pthread_mutex_init(&stdoutMutex, NULL);

    pthread_mutex_t fsmMutex;
    pthread_mutex_init(&fsmMutex, NULL);

    progInt->threads->stdoutMutex = &stdoutMutex;
    progInt->threads->fsmMutex = &fsmMutex;

    progInt->threads->senderEmptyQueueCond = &senderEmptyQueueCond;
    progInt->threads->senderEmptyQueueMutex = &senderEmptyQueueMutex;

    progInt->threads->rec2SenderCond = &rec2SenderCond;
    progInt->threads->rec2SenderMutex = &rec2SenderMutex;

    progInt->threads->mainCond = &mainCond;
    progInt->threads->mainMutex = &mainMutex;

    NetworkConfig netConfig;
    progInterface.netConfig = &netConfig;

    CommunicationDetails comDetails;
    progInterface.comDetails = &comDetails;
    progInt->comDetails->msgCounter = 0;

    bufferInit(&(progInt->comDetails->displayName));
    bufferInit(&(progInt->comDetails->channelID));

    // ------------------------------------------------------------------------
    // Process CLI arguments from user
    // ------------------------------------------------------------------------

    // Use buffer for storing ip address, 
    // then used for storing client input / commanads
    Buffer ipAddress; 
    bufferInit(&ipAddress);

    DEFAULT_NETWORK_CONFIG(progInt->netConfig);

    processArguments(argc, argv, &(progInt->netConfig->protocol), &ipAddress, 
                    &(progInt->netConfig->portNumber), &(progInt->netConfig->udpTimeout), 
                    &(progInt->netConfig->udpMaxRetries));
    
    if(progInt->netConfig->protocol == prot_ERR)
    { 
        errHandling("Argument protocol (-t udp / tcp) is mandatory!", 1); /*TODO:*/
    }
    if(ipAddress.data == NULL)
    {
        errHandling("Server address is (-s address) is mandatory", 1); /*TODO:*/ 
    }

    // ------------------------------------------------------------------------
    // Get server information, create socket
    // ------------------------------------------------------------------------

    // open socket for comunication on client side (this)
    progInterface.netConfig->openedSocket = getSocket(progInt->netConfig->protocol);
    
    struct sockaddr_in address = findServer(ipAddress.data, progInt->netConfig->portNumber);

    // get server address
    progInt->netConfig->serverAddress = (struct sockaddr*) &address;
    progInt->netConfig->serverAddressSize = sizeof(address);

    // ------------------------------------------------------------------------
    // Setup second thread that will handle data receiving
    // ------------------------------------------------------------------------

    pthread_t protReceiver;
    pthread_create(&protReceiver, NULL, protocolReceiver, progInt);

    pthread_t protSender;
    pthread_create(&protSender, NULL, protocolSender, progInt);

    // ------------------------------------------------------------------------
    // Loop of communication
    // ------------------------------------------------------------------------

    // reuse ipAddress buffer which is already initialized and unused
    Buffer* clientInput = &ipAddress; 
    userCommandHandling(progInt, clientInput);

    debugPrint(stdout, "DEBUG: Communicaton ended with %u messages\n", (comDetails.msgCounter - 1));

    // ------------------------------------------------------------------------
    // Clean up resources 
    // ------------------------------------------------------------------------

    pthread_join(protReceiver, NULL);
    pthread_join(protSender, NULL);

    shutdown(progInt->netConfig->openedSocket, SHUT_RDWR);
    free(comDetails.displayName.data);
    free(comDetails.channelID.data);
    free(clientInput->data);
    queueDestroy(progInt->threads->sendingQueue);
    
    pthread_mutex_destroy(&rec2SenderMutex);
    pthread_cond_destroy(&rec2SenderCond);
    pthread_mutex_destroy(&senderEmptyQueueMutex);
    pthread_cond_destroy(&senderEmptyQueueCond);
    pthread_mutex_destroy(&mainMutex);
    pthread_cond_destroy(&mainCond);

    pthread_mutex_destroy(&stdoutMutex);
    pthread_mutex_destroy(&fsmMutex);

    return 0;
}