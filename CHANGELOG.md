## Brief
IPK24CHAT-client is a command-line application for communicating with the server that is using the same (IPK24CHAT) protocol. Protocol is based on UDP or TCP protocol which can be changed with program arguments when the program is being started.

## Functionality
The program has full functionality and is corresponding to assignment specifications. The implementation uses asynchronous threads that communicate using thread mutexes, this makes the program "truly" asynchronous, however, it leads to a more complicated program structure. The program has added command /exit which sets the program to an ending state and ends it (in a similar way it would be ended if EOF was found).

## Known limitations
Because the program is implemented using three asynchronous threads there might be a state that wasn't tested properly, therefore it might get stuck in certain scenarios. Known problems are especially when input is being fed from a file and the server is running locally, this creates an environment where inputs are being processed too quickly by both client and server, resulting in states not accounted for and therefore program freezing (being deadlocked).