// #include "netinet/ether.h"
// #include "netinet/tcp.h"
// #include "netinet/udp.h"
// #include "netinet/ip_icmp.h"

#include "getopt.h"
#include "../tests/clientXserver.h"

#include "customtypes.h"
#include "buffer.h"
#include "utils.h"
#include "networkCom.h"
#include "ipk24protocol.h"

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
            size_t optLen = strlen(optarg);
            bufferResize(ipAddress, optLen);
            stringReplace(ipAddress->data, optarg, optLen);
            break;
        case 'p': *portNum = (uint16_t)atoi(optarg);
            break;
        case 'd': *udpTimeout = (uint16_t)atoi(optarg);
            break;
        case 'r': *udpRetrans = (uint8_t)atoi(optarg);
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
    // Process arguments from user
    // ------------------------------------------------------------------------

    char defaultHostname[] = "127.0.0.1"; /*"anton5.fit.vutbr.cz"*/
    uint16_t defaultPort = PORT_NUMBER; /*4567*/ 

    // Use buffer for storing ip address
    Buffer clientCommands; bufferReset(&clientCommands);

    enum Protocols protocol;
    uint16_t portNum = 0;
    uint16_t udpTimeout = 0;
    uint8_t udpRetransmissions = 0;
    processArguments(argc, argv, &protocol, &clientCommands, &portNum, &udpTimeout, &udpRetransmissions);

    // ------------------------------------------------------------------------
    // Get server information and create socket
    // ------------------------------------------------------------------------

    // open socket for comunication on client side (this)
    int clientSocket = getSocket(protocol);

    char* serverHostname = (clientCommands.data != NULL)? clientCommands.data : defaultHostname;
    uint16_t serverPort = (portNum != 0)? portNum : defaultPort;
    // get server address
    struct sockaddr_in address = findServer(serverHostname, serverPort);
    struct sockaddr* serverAddress = (struct sockaddr*) &address;
    int addressSize = sizeof(address);

    // ------------------------------------------------------------------------
    // Declaring and initializing variables need for communication
    // ------------------------------------------------------------------------

    int flags = 0; //TODO: check if some usefull flags could be done
    cmd_t cmdType; // variable to store current command typed by user
    Buffer protocolMsg; bufferReset(&protocolMsg);
    // Buffer clientCommands; bufferReset(&clientCommands); // Moved up for reusability
    
    CommunicationDetails comDetails;
    comDetails.msgCounter = 0;
    bufferReset(&(comDetails.displayName));
    bufferReset(&(comDetails.channelID));

    BytesBlock commands[4];

    // ------------------------------------------------------------------------
    // Loop of communication
    // ------------------------------------------------------------------------

    do 
    {
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
        bool canBeSended = assembleProtocol(cmdType, commands, &protocolMsg, &comDetails);

        int bytesTx; // number of sended bytes
        if(canBeSended)
        {
            // send buffer to the server 
            bytesTx = sendto(clientSocket, clientCommands.data, clientCommands.used, flags, serverAddress, addressSize);

            if(bytesTx < 0)
            {
                errHandling("Sending bytes was not successful", 1); // TODO: change error code
            }
            // increase number of messages recevied, only if message was sent
            comDetails.msgCounter += 1;
        }

        // Exit loop if /exit detected 
        if(cmdType == CMD_EXIT) { break; }
    } while (1);

    printf("DEBUG: Communicaton ended with %u messages\n", comDetails.msgCounter); //DEBUG: delete

    // ------------------------------------------------------------------------
    // Closing up communication
    // ------------------------------------------------------------------------

    shutdown(clientSocket, SHUT_RDWR);
    free(comDetails.displayName.data);
    free(comDetails.channelID.data);
    free(clientCommands.data);
    free(protocolMsg.data);

    return 0;
}