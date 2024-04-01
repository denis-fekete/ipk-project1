/**
 * @file main.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief Main program of IPK24CHAT-CLIENT application meant for communication 
 * with server over network using IPK24CHAT protocol that is based on TCP or 
 * UDP protocols. Program works multiple threads and mutexes to allow correct 
 * handling of events invoked eather by user of server that client is  
 * connected to.
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "getopt.h" // argument processing
#include "signal.h"

#include "protocolReceiver.h"
#include "protocolSender.h"
#include "libs/cleanUpMaster.h"

#ifdef DEBUG
    // global variable for printing to debug, only if DEBUG is defined
    pthread_mutex_t debugPrintMutex = PTHREAD_MUTEX_INITIALIZER;
#endif

// global pointer of ProgramInterface, this is needed to ensure correct 
// closing of program in case of SIGINT
ProgramInterface* globalProgInt;

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Prints msg and exits program with errorCode
 * 
 * Error code: <br>
 * 1     - initialization errors
 * 11    - network / connection to server
 * 99       - memory allocation error
 * 98       - internal invalid values passed to function
 * 
 * @param msg Message to be printed
 * @param errorCode Error code that will be used as exit code
 * @return int Returns 0
 */
int errHandling(const char* msg, int errorCode)
{
    switch (errorCode)
    {
    // program communication already begun, stop it and exit
    case err_INTERNAL_UNEXPECTED_RESULT:
    case err_INTERNAL_BAD_ARG:
        // set program state to fsm_ERR
        setProgramState(globalProgInt, fsm_ERR);
        // print error message
        fprintf(stderr, "ERR: %s\n", msg);
        // signal main thread (stdin handling) to wake up and stop 
        pthread_cond_signal(globalProgInt->threads->mainCond);
        // empty whole queue
        queueLock(globalProgInt->threads->sendingQueue);
        queuePopAllMessages(globalProgInt->threads->sendingQueue);
        queueUnlock(globalProgInt->threads->sendingQueue);
        // add bye to message queue
        sendBye(globalProgInt);
        // wait on sender to send signal, to make sure program interface is not destroy before bye was send
        pthread_cond_wait(globalProgInt->threads->mainCond, 
            globalProgInt->threads->mainMutex);

        // destroy program interface
        programInterfaceDestroy(globalProgInt);
        break;
    // communication was not properly set, don't end it 
    case err_MISING_PROGRAM_ARG:
    case err_NETWORK_INIT:
    case err_COMMUNICATION:
    case err_MEMORY_FAIL: // cannot know if communication was already established or not
    case err_NO_ERR:
        fprintf(stderr, "ERR: %s\n", msg);
        // destroy program interface
        programInterfaceDestroy(globalProgInt);
        break;
    }

    exit(errorCode);
    return 0; // anti-compiler-error solution
}

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
            printCliHelpMenu("ipk24chat-client");
            errHandling("", 0);
            break;
        case 't':
            if(strcmp(optarg, "udp") == 0) { *prot = prot_UDP; }
            else if(strcmp(optarg, "tcp") == 0) { *prot = prot_TCP; }
            else
            {
                errHandling("Unknown protocol provided in -t option. Use -h for help", err_MISING_PROGRAM_ARG);
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
            errHandling("Unknown option. Use -h for help", err_MISING_PROGRAM_ARG);
            break;
        }
    }
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

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
        // commands: CMD, USERNAME, SECRET, DISPLAYNAME
        bufferResize(&(progInt->comDetails->displayName), pBlocks->cmd_auth_displayname.len + 1);

        stringReplace(  progInt->comDetails->displayName.data, 
                        pBlocks->cmd_auth_displayname.start, 
                        pBlocks->cmd_auth_displayname.len);

        progInt->comDetails->displayName.used = pBlocks->cmd_auth_displayname.len;
        progInt->comDetails->displayName.data[pBlocks->cmd_auth_displayname.len] = '\0';

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
        break;
    case cmd_JOIN:
        // can only be send in open state
        if(getProgramState(progInt) == fsm_OPEN)
        {
            // commands: CMD, CHANNELID
            bufferResize(&(progInt->comDetails->channelID), pBlocks->cmd_join_channelID.len + 1);

            stringReplace(  progInt->comDetails->channelID.data, 
                            pBlocks->cmd_join_channelID.start, 
                            pBlocks->cmd_join_channelID.len);
            progInt->comDetails->channelID.used = pBlocks->cmd_join_channelID.len;
            progInt->comDetails->channelID.data[progInt->comDetails->channelID.used] = 0;
            return true; // send join message
        }
        break;
    case cmd_RENAME:
        // can only be send in open state
        if(getProgramState(progInt) == fsm_OPEN)
        {
            // replace displayname stored in Communication Details with data 
            // from user provided command
            bufferResize(   &(progInt->comDetails->displayName), 
                            pBlocks->cmd_rename_displayname.len + 1);
            stringReplace(  progInt->comDetails->displayName.data, 
                            pBlocks->cmd_rename_displayname.start, 
                            pBlocks->cmd_rename_displayname.len);
            progInt->comDetails->displayName.used = pBlocks->cmd_rename_displayname.len;
            progInt->comDetails->displayName.data[progInt->comDetails->displayName.used] = 0;
            return false; // rename is local only, dont send
        }
        break;
    case cmd_MSG:
        if(getProgramState(progInt) == fsm_OPEN)
        {
            return true; // send message
        }
        break;;
    case cmd_EXIT:
        return true;
    case cmd_NONE:
        return false;
    case cmd_MISSING:
        safePrintStderr("ERR: Bad command argument / not enough arguments.\n");
        return false;
    default: 
        break;
    }
    safePrintStderr("ERR: You are not connected to server! "
            "Use /auth to connect to server or /help for more information.\n"); 
    return false;
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Main loop for user input
 * 
 * @param progInt Pointer to the program interface
 */
void userCommandHandling(ProgramInterface* progInt)
{
    // use already allocated buffers for global freeing in case of SIGINT
    Buffer* protocolMsg = &(progInt->cleanUp->protocolToSendedByMain);
    Buffer* clientInput = &(progInt->cleanUp->clientInput);

    int canBeSended;
    bool eofDetected = false;
    msg_flags flags = msg_flag_NONE;
    ProtocolBlocks pBlocks;

    // main shall stop to work in these states: fsm_ERR, fsm_SIGINT_BYE, fsm_END
    while (getProgramState(progInt) < fsm_EMPTY_Q_BYE)
    {
        debugPrint(stdout, "DEBUG: Main resumed\n");

        // eof was detected in last loop, send bye and exit
        if(eofDetected)
        {
            // set program to empty queue and leave
            setProgramState(progInt, fsm_EMPTY_Q_BYE);
            // add bye to the message queue
            sendBye(progInt);
            // wake up sender to exit
            pthread_cond_signal(progInt->threads->senderEmptyQueueCond);
            continue;
        }

        // --------------------------------------------------------------------
        // Convert user input into an protocol
        // --------------------------------------------------------------------
        canBeSended = false;
        // Load buffer from stdin, store length of buffer
        clientInput->used = loadBufferFromStdin(clientInput, &eofDetected);
        // Separate clientCommands buffer into commands (ByteBlocks),
        // store recognized command
        flags = msg_flag_NONE;
        canBeSended = userInputToCmds(clientInput, &pBlocks, &flags);

        // Filter commands by type and FSM state
        canBeSended = filterCommandsByFSM(&pBlocks, progInt, &flags);

        // if message should not be send skip it because it is local only
        if(!canBeSended) { continue; }

        // Assembles array of bytes into Buffer protocolMsg, returns if 
        // message can be trasmitted
        UDP_VARIANT
            canBeSended = assembleProtocolUDP(&pBlocks, protocolMsg, progInt);
        TCP_VARIANT
            canBeSended = assembleProtocolTCP(&pBlocks, protocolMsg, progInt);
        END_VARIANTS
        // if message wasnt assebled correcttly
        if(!canBeSended) { continue; }
        
        // add message to the queue
        queueLock(progInt->threads->sendingQueue);

        // if queue is empty store that sender should be signaled
        bool signalSender = false;
        if(queueIsEmpty(progInt->threads->sendingQueue)) { signalSender = true; }
        
        queueAddMessage(progInt->threads->sendingQueue, protocolMsg, flags, pBlocks.type);
        // signal sender if he is waiting because queue is empty
        if(signalSender)
        {
            pthread_cond_signal(progInt->threads->senderEmptyQueueCond);
        }
        queueUnlock(progInt->threads->sendingQueue);

                    
        // Exit loop if /exit detected 
        if(pBlocks.type == cmd_EXIT || pBlocks.type == msg_BYE)
        {
            // set state to empty queue, send bye and exit
            setProgramState(progInt, fsm_EMPTY_Q_BYE);
            // wake up sender to exit
            pthread_cond_signal(progInt->threads->senderEmptyQueueCond);
        }

        debugPrint(stdout, "DEBUG: Main waiting\n");
        // wait for message to be processed/confirmed
        pthread_cond_wait(progInt->threads->mainCond, progInt->threads->mainMutex);
    }
}


// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // ------------------------------------------------------------------------
    // Initialize interface for program
    // ------------------------------------------------------------------------

    ProgramInterface* progInt = (ProgramInterface*) calloc(1, sizeof(ProgramInterface));
    if(progInt == NULL)
    {
        fprintf(stderr, "Failed to initialize program interface");
        exit(1);
    }

    // initialize Program Interface
    programInterfaceInit(progInt);
    // set global program interface for SIGINT handling
    globalProgInt = progInt;

    // SIGINT handling
    signal(SIGINT, sigintHandler);

    // ------------------------------------------------------------------------
    // Process CLI arguments from user
    // ------------------------------------------------------------------------

    // Reuse clientInput Buffer that is not used currently
    Buffer* ipAddress = &(progInt->cleanUp->clientInput); 

    defaultNetworkConfig(progInt->netConfig);

    processArguments(argc, argv, &(progInt->netConfig->protocol), ipAddress, 
                    &(progInt->netConfig->portNumber), &(progInt->netConfig->udpTimeout), 
                    &(progInt->netConfig->udpMaxRetries));
    if(progInt->netConfig->protocol == prot_ERR)
    { 
        errHandling("Argument protocol (-t udp / tcp) is mandatory!", err_MISING_PROGRAM_ARG);
    }
    if(ipAddress->data == NULL)
    {
        errHandling("Server address is (-s address) is mandatory", err_MISING_PROGRAM_ARG); 
    }

    // ------------------------------------------------------------------------
    // Get server information, create socket
    // ------------------------------------------------------------------------

    // open socket for comunication on client side (this)
    progInt->netConfig->openedSocket = getSocket(progInt->netConfig->protocol);
    
    struct sockaddr_in address = findServer(ipAddress->data, progInt->netConfig->portNumber);

    // get server address
    progInt->netConfig->serverAddress = (struct sockaddr*) &address;
    progInt->netConfig->serverAddressSize = sizeof(address);

    UDP_VARIANT

    TCP_VARIANT
        const int res = connect( progInt->netConfig->openedSocket, 
                    progInt->netConfig->serverAddress,
                    progInt->netConfig->serverAddressSize);
        if( res != 0 ) {
            errHandling("Failed to connect to the server", err_NETWORK_INIT);
        } 
    END_VARIANTS

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

    userCommandHandling(progInt);

    // ------------------------------------------------------------------------
    // Clean up resources 
    // ------------------------------------------------------------------------

    pthread_join(protReceiver, NULL);
    pthread_join(protSender, NULL);

    debugPrint(stdout, "DEBUG: Communicaton ended with %u messages\n", 
        ((progInt->comDetails->msgCounter > 0)? 0 : progInt->comDetails->msgCounter - 1));

    // close socket
    shutdown(progInt->netConfig->openedSocket, SHUT_RDWR);
    // destroy program interface
    programInterfaceDestroy(progInt);
    return 0;
}