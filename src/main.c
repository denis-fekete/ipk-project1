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

#include "libs/customtypes.h"
#include "libs/buffer.h"
#include "libs/utils.h"
#include "libs/networkCom.h"
#include "libs/ipk24protocol.h"
#include "libs/msgQueue.h"

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
            bufferResize(ipAddress, optLen);
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
 * based on cmdType provided.  
 * 
 * @note Stored data is copied, so commands can be safely rewritten
 * 
 * @param cmdType Recognized command from user 
 * @param commands Array of commands
 * @param comDetails Pointer to the CommunicationDetails variable that will 
 * be updated
 */
void storeInformation(cmd_t cmdType, BytesBlock commands[4], CommunicationDetails* comDetails)
{
    switch (cmdType)
    {
    case cmd_AUTH:
        // commands: CMD, USERNAME, SECRET, DISPLAYNAME
        bufferResize(&(comDetails->displayName), commands[3].len + 1);

        stringReplace(comDetails->displayName.data, commands[3].start, commands[3].len);
        comDetails->displayName.data[commands[3].len] = '\0';
        comDetails->displayName.used = commands[3].len;
        break;
    case cmd_JOIN:
        // commands: CMD, CHANNELID
        bufferResize(&(comDetails->channelID), commands[1].len + 1);

        stringReplace(comDetails->channelID.data, commands[1].start, commands[1].len);
        comDetails->channelID.used = commands[1].len;
        break;
    case cmd_RENAME:
        // commands: CMD, DISPLAYNAME
        bufferResize(&(comDetails->displayName), commands[1].len + 1);
        stringReplace(comDetails->displayName.data, commands[1].start, commands[1].len);
        comDetails->displayName.data[commands[1].len] = '\0';
        comDetails->displayName.used = commands[1].len;
        break;;
    default: break;
    }
}

typedef enum FilterResult {fres_ERR, fres_SKIP, fres_STORE, fres_SEND} filter_res_t;

filter_res_t filterCommandsByFSM(cmd_t cmdType, ProgramInterface* progInt, BytesBlock commands[4], msg_flags* flags)
{
    // if buffer is empty ... newline was entered
    if(cmdType == cmd_NONE) return fres_SKIP;

    switch (getProgramState(progInt))
    {
    case fsm_START:
        // --------------------------------------------------------------------
        switch (cmdType)
        {
        case cmd_AUTH:
            storeInformation(cmdType, commands, progInt->comDetails);
            *flags = msg_flag_AUTH;
            setProgramState(progInt, fsm_AUTH_W82_BE_SENDED);
            return fres_SEND;
        case cmd_HELP:
            printUserHelpMenu(progInt);
            return fres_SKIP;
        case cmd_RENAME:
            storeInformation(cmdType, commands, progInt->comDetails);
            return fres_SKIP;
        default:
            safePrintStdout("System: You are not connected to server! "
                "Use /auth to connect to server or /help for more information.\n"); 
            return fres_SKIP;
            break;
        }
        // --------------------------------------------------------------------
    default:
        storeInformation(cmdType, commands, progInt->comDetails);
        return fres_SEND;
        break;
    }

    // should this message be not send?
    return fres_ERR;
}
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

//DEBUG:
#define CMD_EXIT_SKIP_START if(cmdType != CMD_EXIT) {
#define CMD_EXIT_SKIP_END }

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
    MessageQueue receivedQueue;
    queueInit(&sendingQueue); // queue of outcoming (user sent) messages
    queueInit(&receivedQueue); // queue of incoming (server sent) messages

    progInt->threads->sendingQueue = &sendingQueue;
    progInt->threads->receivedQueue = &receivedQueue;
    
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

    // ------------------------------------------------------------------------
    // Process arguments from user
    // ------------------------------------------------------------------------

    #ifdef DEBUG
        char defaultHostname[] = "127.0.0.1"; /*"anton5.fit.vutbr.cz"*/
    #endif

    // Use buffer for storing ip address, 
    // then used for storing client input / commanads
    Buffer clientCommands; // 
    bufferInit(&clientCommands);

    DEFAULT_NETWORK_CONFIG(progInt->netConfig);

    processArguments(argc, argv, &(progInt->netConfig->protocol), &clientCommands, 
                    &(progInt->netConfig->portNumber), &(progInt->netConfig->udpTimeout), 
                    &(progInt->netConfig->udpMaxRetries));
    
    if(progInt->netConfig->protocol == prot_ERR)
    { 
        errHandling("Argument protocol (-t udp / tcp) is mandatory!", 1); /*TODO:*/
    }
    #ifndef DEBUG
        if(clientCommands.data == NULL) { errHandling("Server address is (-s address) is mandatory", 1); /*TODO:*/ }
    #endif
    // ------------------------------------------------------------------------
    // Get server information and create socket
    // ------------------------------------------------------------------------

    // open socket for comunication on client side (this)
    progInterface.netConfig->openedSocket = getSocket(progInt->netConfig->protocol);
    
    #ifdef DEBUG
        char* serverHostname = (clientCommands.data != NULL)? clientCommands.data : defaultHostname;
        struct sockaddr_in address = findServer(serverHostname, progInt->netConfig->portNumber);
    #else
        struct sockaddr_in address = findServer(clientCommands.data, progInt->netConfig->portNumber);
    #endif

    // get server address
    progInt->netConfig->serverAddress = (struct sockaddr*) &address;
    progInt->netConfig->serverAddressSize = sizeof(address);

    // ------------------------------------------------------------------------
    // Declaring and initializing variables need for communication
    // ------------------------------------------------------------------------

    cmd_t cmdType; // variable to store current command typed by user

    Buffer protocolMsg;
    bufferInit(&protocolMsg);

    // Buffer clientCommands; bufferInit(&clientCommands); // Moved up for reusability
    
    progInt->comDetails->msgCounter = 0;

    bufferInit(&(progInt->comDetails->displayName));
    bufferInit(&(progInt->comDetails->channelID));

    BytesBlock commands[4];

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

    // continue while continueProgram is true, only main id can go into this loop
    // while (progInt->threads->continueProgram)
    while (getProgramState(progInt) != fsm_END)
    {
        // --------------------------------------------------------------------
        // Convert user input into an protocol
        // --------------------------------------------------------------------
        bool canBeSended;
        bool eofDetected = false;
        // Load buffer from stdin, store length of buffer
        clientCommands.used = loadBufferFromStdin(&clientCommands, &eofDetected);
        // Separate clientCommands buffer into commands (ByteBlocks),
        // store recognized command
        cmdType = userInputToCmds(&clientCommands, commands, &eofDetected);

        msg_flags flags = msg_flag_MSG;
        filter_res_t result =  filterCommandsByFSM(cmdType, progInt, commands, &flags);

        switch (result)
        {
        case fres_SKIP: continue; break;
        case fres_SEND: break;
        default:
            break;
        }

        // Assembles array of bytes into Buffer protocolMsg, returns if 
        // message can be trasmitted
        canBeSended = assembleProtocol(cmdType, commands, &protocolMsg, progInt);

        if(canBeSended)
        {
            // // add message to the queue
            // queueAddMessage(progInt->threads->sendingQueue, &protocolMsg, flags);
            // comDetails.msgCounter += 1;

            // if sendingQueue is empty, sender is asleep waiting
            // wake it up
            if(queueIsEmpty(progInt->threads->sendingQueue))
            {
                pthread_cond_signal(&senderEmptyQueueCond);
            }
            // TODO: swap with above
            // add message to the queue
            queueAddMessage(progInt->threads->sendingQueue, &protocolMsg, flags);
        }
        
        // Exit loop if /exit detected 
        if(cmdType == cmd_EXIT)
        {
            // wait for sender to ping that it is empty
            pthread_cond_wait(&mainCond, &mainMutex);
            // set state to empty queue, send bye and exit
            setProgramState(progInt, fsm_EMPTY_Q_BYE);
            // wake up sender to exit
            pthread_cond_signal(&senderEmptyQueueCond);
        }
    }

    debugPrint(stdout, "DEBUG: Communicaton ended with %u messages\n", comDetails.msgCounter);
    // ------------------------------------------------------------------------
    // Closing up communication
    // ------------------------------------------------------------------------

    pthread_join(protReceiver, NULL);
    pthread_join(protSender, NULL);

    debugPrint(stdout, "Main ended\n");

    shutdown(progInt->netConfig->openedSocket, SHUT_RDWR);
    free(comDetails.displayName.data);
    free(comDetails.channelID.data);
    free(clientCommands.data);
    free(protocolMsg.data);
    queueDestroy(progInt->threads->sendingQueue);
    queueDestroy(progInt->threads->receivedQueue);

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