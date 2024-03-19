/**
 * @file utils.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "utils.h"

int errHandling(const char* msg, int errorCode)
{
    fprintf(stderr, "ERROR: %s\n", msg);
    exit(errorCode);
    return 0;
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

long findZeroInString(char* string, size_t len)
{
    long index = 0;
    for(; ((size_t)index < len) && (string[index] != '\0') ; index++) {}

    // -1 to return index of last character before \0
    index = index - 1;

    if(index < 0)
    {
        if(index == -1) { return 0; }
        else
        {
            return errHandling("Intrnal error in findZeroInString()", 1); // TODO: change error code
        }
    }
    else { return index; }
}


// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

void getWord(BytesBlock* block, char* startOfLastWord, size_t bufferSize)
{
    if(block == NULL || startOfLastWord == NULL || bufferSize <= 0)
    {
        errHandling("Invalid input variables in fuction getWord()", 1); // TODO: change err code
    }

    size_t index = skipBlankCharsInString(startOfLastWord, bufferSize);
    // +1 because 
    block->start = &(startOfLastWord[index]);

    index = findBlankCharInString(block->start, bufferSize - block->len);
    block->len = index;
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

long findBlankCharInString(char* string, size_t len)
{
    size_t index = 0;
    for(; index <= len ; index++)
    {
        if(string[index] == ' ' || string[index] == '\t' || string[index] == '\0' || string[index] == '\n')
        { 
            return index;
        }
    }

    // Character was not found
    return -1;
}



long skipBlankCharsInString(char* string, size_t len)
{
    size_t index = 0;
    for(; index < len ; index++)
    {
        if(! (string[index] == ' ' || string[index] == '\t'))
        { 
            return index;
        }

        if(string[index] == '\0' || string[index] == '\n')
        {
            // Character was not found
            return -1;
        }
    }

    // Character was not found
    return -1;
}

int isEndingCharacter(char input)
{
    // 3 = Ctrl-D, 4 = Ctrl-C
    if(input == EOF || input == '\n' || input == 3 || input == 4)
    {
        return 0;
    }

    return 1;
}


// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

void stringReplace(char* dst, char* src, size_t len)
{
    if(dst != NULL && src != NULL)
    {
        for(size_t i = 0; i < len; i++)
        {
            dst[i] = src[i];
        }
        return;
    }

    #ifdef DEBUG
        fprintf(stderr, "Error: stringReplace() received bad pointer or "
        "invalid length (dst:%p, src:%p, len:%ld\n", dst, src, len);
    #endif
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

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

/**
 * @brief Adds ONE to message counter
 * 
 * @param newState New state to be set
 */
void incMessageCounter(ProgramInterface* progInt)
{
    if(progInt){}
}

/**
 * @brief Returns message counter
 */
uint16_t getMessageCounter(ProgramInterface* progInt)
{
    if(progInt){}
    return 1;

}