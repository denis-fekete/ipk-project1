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
#include "pthread.h"

#include "libs/customtypes.h"
#include "libs/buffer.h"
#include "libs/utils.h"
#include "libs/networkCom.h"
#include "libs/ipk24protocol.h"
#include "libs/msgQueue.h"

#include "unistd.h"

#include "protocolReceiver.h"
#include "protocolSender.h"

// Global variable shared between files to signalize whenever loops should continue
bool continueProgram = true;
MessageQueue* sendingQueue; // queue of outcoming (user sent) messages
MessageQueue* receivedQueue; // queue of incoming (server sent) messages
NetworkConfig config; // store network configuration / settings 
pthread_cond_t pingSenderCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t pingSenderMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t pingMainCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t pingMainMutex = PTHREAD_MUTEX_INITIALIZER;

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

void printHelpMenu()
{
    printf("TODO: print menu\n");
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
            printHelpMenu();
            exit(EXIT_SUCCESS);
            break;
        case 't':
            if(strcmp(optarg, "udp") == 0) { *prot = UDP; }
            else if(strcmp(optarg, "tcp") == 0) { *prot = TCP; }
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
    case AUTH:
        // commands: CMD, USERNAME, SECRET, DISPLAYNAME
        bufferResize(&(comDetails->displayName), commands[3].len + 1);

        stringReplace(comDetails->displayName.data, commands[3].start, commands[3].len);
        comDetails->displayName.data[commands[3].len] = '\0';
        comDetails->displayName.used = commands[3].len;
        break;
    case JOIN:
        // commands: CMD, CHANNELID
        bufferResize(&(comDetails->channelID), commands[1].len + 1);

        stringReplace(comDetails->channelID.data, commands[1].start, commands[1].len);
        comDetails->channelID.used = commands[1].len;
        break;
    case RENAME:
        // commands: CMD, DISPLAYNAME
        bufferResize(&(comDetails->displayName), commands[1].len + 1);
        stringReplace(comDetails->displayName.data, commands[1].start, commands[1].len);
        comDetails->displayName.data[commands[1].len] = '\0';
        comDetails->displayName.used = commands[1].len;
        break;;
    default: break;
    }
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // ------------------------------------------------------------------------
    //
    // ------------------------------------------------------------------------

    continueProgram = true;

    sendingQueue = (MessageQueue*) malloc(sizeof(MessageQueue));
    if(sendingQueue == NULL) {errHandling("Failed malloc for sendingQueue", 1); /*TODO:*/};
    receivedQueue = (MessageQueue*) malloc(sizeof(MessageQueue));
    if(receivedQueue == NULL) {errHandling("Failed malloc for receivedQueue", 1); /*TODO:*/};

    queueInit(sendingQueue); // queue of outcoming (user sent) messages
    queueInit(receivedQueue); // queue of incoming (server sent) messages

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

    DEFAULT_NETWORK_CONFIG(config);

    processArguments(argc, argv, &(config.protocol), &clientCommands, 
                    &(config.portNumber), &(config.udpTimeout), 
                    &(config.udpMaxRetries));
    
    if(config.protocol == ERR)
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
    config.openedSocket = getSocket(config.protocol);

    #ifdef DEBUG
        char* serverHostname = (clientCommands.data != NULL)? clientCommands.data : defaultHostname;
        struct sockaddr_in address = findServer(serverHostname, config.portNumber);
    #else
        struct sockaddr_in address = findServer(clientCommands.data, config.portNumber);
    #endif

    // get server address
    config.serverAddress = (struct sockaddr*) &address;
    config.serverAddressSize = sizeof(address);

    // ------------------------------------------------------------------------
    // Declaring and initializing variables need for communication
    // ------------------------------------------------------------------------

    cmd_t cmdType; // variable to store current command typed by user

    Buffer protocolMsg;
    bufferInit(&protocolMsg);

    // Buffer clientCommands; bufferInit(&clientCommands); // Moved up for reusability
    
    CommunicationDetails comDetails;
    comDetails.msgCounter = 0;

    bufferInit(&(comDetails.displayName));
    bufferInit(&(comDetails.channelID));

    BytesBlock commands[4];

    // ------------------------------------------------------------------------
    // Setup second thread that will handle data receiving
    // ------------------------------------------------------------------------

    // get id of main thread before any threads are called
    pthread_t mainID = pthread_self();

    config.comDetails = &comDetails;

    pthread_t protReceiver;
    pthread_create(&protReceiver, NULL, protocolReceiver, &config);

    pthread_t protSender;
    pthread_create(&protSender, NULL, protocolSender, &config);

    // get current thread id
    pthread_t threadID = pthread_self();


    // ------------------------------------------------------------------------
    // Loop of communication
    // ------------------------------------------------------------------------
    // continue while continueProgram is true, only main id can go into this loop
    while (continueProgram && threadID == mainID)
    {
        // --------------------------------------------------------------------
        // Convert user input into an protocol
        // --------------------------------------------------------------------
        bool canBeSended;
        // Load buffer from stdin, store length of buffer
        clientCommands.used = loadBufferFromStdin(&clientCommands);
        // Separate clientCommands buffer into commands (ByteBlocks),
        // store recognized command
        cmdType = userInputToCmds(&clientCommands, commands);
        if(cmdType == NONE) { continue; }
        // store commands into comDetails for future use in other commands
        storeInformation(cmdType, commands, &comDetails);
        // Assembles array of bytes into Buffer protocolMsg, returns if 
        // message can be trasmitted   
        canBeSended = assembleProtocol(cmdType, commands, &protocolMsg, &comDetails);

        if(canBeSended)
        {
            // if sendingQueue is empty, sender is asleep waiting
            // wake him up
            if(queueIsEmpty(sendingQueue))
            {
                pthread_cond_signal(&pingSenderCond);
            }
            // add message to the queue
            queueAddMessage(sendingQueue, &protocolMsg);
        }
        
        for(size_t i = 0; i < protocolMsg.allocated; i++)
        { protocolMsg.data[i] = 0; }

        // Exit loop if /exit detected 
        if(cmdType == CMD_EXIT)
        {
            // wake up sender to exit
            pthread_cond_signal(&pingSenderCond);
            continueProgram = false;
        }
    }

    #ifdef DEBUG
        printf("DEBUG: Communicaton ended with %u messages\n", comDetails.msgCounter); //DEBUG: delete
    #endif
    // ------------------------------------------------------------------------
    // Closing up communication
    // ------------------------------------------------------------------------

    printf("main thread ended loop\n");
    pthread_join(protReceiver, NULL);
    pthread_join(protSender, NULL);

    shutdown(config.openedSocket, SHUT_RDWR);
    free(comDetails.displayName.data);
    free(comDetails.channelID.data);
    free(clientCommands.data);
    free(protocolMsg.data);
    queueDestroy(sendingQueue);
    queueDestroy(receivedQueue);
    free(sendingQueue);
    free(receivedQueue);

    // pthread_mutex_destroy(&pingSenderMutex);
    // pthread_mutex_destroy(&pingMainMutex);
    return 0;
}