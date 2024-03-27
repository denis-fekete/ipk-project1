#include "programInterface.h"

/**
 * @brief Prints help menu when user inputs /help command 
 */
void printCliHelpMenu(const char* executableName)
{
    printf(
        "Usage: %s [OPTION] [ARGUMENT] ...\n"
        "Starts client for communication with server at provided address "
        "(through -s OPTION) using IPK24-CHAT Protocol based on TCP or "
        "UDP (based on -t OPTION)\n"
        "\n"
        "Mandatory options:\n"
        "\t-s\t- "
        "Sets server IP address (can be in \"www.server.com\" format) "
        "to which client will try to connect\n"
        "\t-t\t- "
        "Sets between UDP or TCP protocol to be used for sending messages to server\n"
        "\nNon-mandatory options:\n"
        "\t-p\t- "
        "Specifies which port will client try to connect to at specified "
        "IP adress. Default value is 4567.\n"
        "\t-d\t- "
        "Sets UDP confirmation timeout in milliseconds\n"
        "\t-r\t- "
        "Sets maximum number of UDP retransmissions\n"
        "\t-h\t- "
        "Prints this help menu end exits program with code 0\n"

        , executableName
    );
}

/**
 * @brief Prints user help menu in running client
 * 
 * @param progInt Pointer to ProgramInterface for thread-safe printing
 */
void printUserHelpMenu(ProgramInterface* progInt)
{
    safePrintStdout(
        "Commands start with \"/\", \"{arg}\" symbolize mandatory arguments for"
        " command, explanation of command is written after \"-\" symbol."
        " Eligable commands are listed below: \n"
        "\n\t/auth {Username} {Secret} {DisplayName}\t\t- "
        "Authenticates user to server with data provided from the arguments. "
        "{Username} and {Secret} are credentials. {Displayname} is name under "
        "which will user(this) send messages to server."
        "\n\t/join {ChannelID}\t\t\t\t- "                 
        "Joins user to channel with {ChannelID}"
        "\n\t/rename {DisplayName}\t\t\t\t- "             
        "Renames user to {Displayname}"
        "\n\t/help\t\t\t\t\t\t- "                     
        "Prints this help message."
        "\n"
        );
}


// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

/**
 * @brief Changes program state to new state with thread protecion using mutex
 * 
 * @param newState New state to be set
 */
void setProgramState(ProgramInterface* progInt, fsm_t newState)
{
    debugPrintSeparator(stdout);
    pthread_mutex_lock(progInt->threads->fsmMutex);
    debugPrint(stdout, "FSM state changed. Old: %i", progInt->threads->fsmState);
    progInt->threads->fsmState = newState;
    pthread_mutex_unlock(progInt->threads->fsmMutex);
    debugPrint(stdout, ", New: %i\n", newState);
    debugPrintSeparator(stdout);
}

/**
 * @brief Returns program state with thread protecion using mutex
 */
fsm_t getProgramState(ProgramInterface* progInt)
{
    pthread_mutex_lock(progInt->threads->fsmMutex);
    fsm_t val =  progInt->threads->fsmState;
    pthread_mutex_unlock(progInt->threads->fsmMutex);

    return val;
}
