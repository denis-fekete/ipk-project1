## Description
IPK24CHAT-client is a command-line application for communicating with the server that is using the same (IPK24CHAT) protocol. Protocol is based on UDP or TCP protocol which can be changed with program arguments when the program is being started.

The program is written in C programming language and has three **threads** (not processes) that are running simultaneously. These threads communicate using **ProgramInterface** Synchronization of threads is done with **mutexes** that make sure that important data structures are protected from being accessed by more than one thread at a time. Protected parts of the program are **MessageQueue**, program state (Fininite State Machine) and standard output. Mutexes can also be used to ensure a certain order of actions (especially in the UDP variant).

### Program Interface
The ProgramInterface is a structure holding information about the whole program and all dynamically allocated memory. This allows the program to correctly react to *SIGINT* signals that can be sent by the operating system. ProgramInterface is part of parameters for the most functions in the program, however, some functions that cannot have parameters use the global variable defined in `main.c` which is a pointer to the main ProgramInterface variable.

### MessageQueue
The MessageQueue is a (custom) library contenting priority FIFO (first in first out) queue structure and functions needed to work with this structure. This structure contains a mutex that allows only one caller to work with the queue at a time. This mutex was part of the functions however due to some limitations it was placed outside of the function and the programmer has to work with this mutex correctly. MessageQueue is used for messages to be sent or that were received (for controlling duplicate messages in the UDP variant). Priority FIFO queue means that queue is always working with the oldest added message, however, some functions can bend behavior.

## Threads and asynchronous communication 
As mentioned before three threads are used in this program, from now on call them **modules**. This is needed to ensure that user input, sending of messages and receiving can be done at the same time, and also ensure that modules are suspended and not taking CPU resources when they have nothing to do. This however has some disadvantages and the program has become inflated, therefore it is not as easily readable it also doesn't allow for one single *"central"* FSM to exist in the program and each module has to have its implementation of FSM (function that controls flow of the program ... switch or if statements).

### Main module/thread
The main module has to initialize ProgramInterace, open the socket and establish network communication. After these steps, the main initializes the **receiver** and **sender** modules. From this point main module's purpose is to take user inputs, convert them to the correct format and add them to the global MessageQueue. After the message is added to the MessageQueue main will be suspended until other modules signal it to work again, allowing the user to perform actions. If the program state changes one of the ending states for main (`fsm_ERR`, `fsm_ERR_W84_CONF`, `fsm_SIGINT_BYE`, `fsm_END_W84_CONF` or `fsm_END`) it stops loading user input from the standard input stream (stdin) and starts waiting for other module to stop. After all other modules stop working ProgramInterface will be destroyed and the program exits.

### Receiver module/thread
The receiver module's job is to receive all messages from the server and change the program state accordingly. After receiving the message from the server it is broken down into commands and based on detected commands action is performed.

### Sender module/thread
The sender module's job is to send all messages from the MessageQueue to the server. In the case of UDP communication, it also handles the correct retransmission of messages alternatively if the message is retransmitted too many times *error* message will be sent to the server and communication will be ended. In UDP all messages must be confirmed, otherwise sender will try to resent them. If the *error* or *bye* message timeout sender will continue as if they were sent and will try to correctly end the program.

## Finite State Machine

### Short explanation
The program starts in the `Start` state and any messages except for `/auth` will lead to an internal error being printed on standard output. After `/auth` has been sent, confirmed, replied to and confirmation of reply was sent the program is set into the `Open` state. From this state user can perform all valid commands. From the `Open` state based on the other modules or user input the program will always end in the `End` state where it will exit.

### UDP
![UDP Finite State Machine](ipk_fsm_udp.drawio.svg)
### TCP
![TCP Finite State Machine](ipk_fsm_tcp.drawio.svg)


### Legend:
 - `|` -> logical or (example msg1 | msg2 == in case msg1 or msg2)
 - `actor:` -> actors that performed action, possible actors stdin, sender, receiver, program
 - `*` -> all other messages that are not explicitly typed
 - `/command` -> command that was used, for list of accepted commands see section commands
 - `"action"` -> description of the action that took place, that cannot be descripted any other way 

### Notes for UDP:
 - between the transition from `Error waiting for confirmation` and `End sent, waiting for confirmation` sender sends the *bye* message
 - in the `Open` state the transition `stdin: *`:
    * can trigger internal error printing, however, these prints are just for user and are not sent
    * messages sent must be confirmed by the receiver, if they are not confirmed after a certain time they are retransmitted again 


## Commands
The client can perform actions based on the provided commands. List of commands:
- `/auth {username} {secret} {displayname}`-> autheticates user to the server and sets displayname
- `/join {channelID}` -> changes the channel that the client is connected to
- `/help` -> prints help menu
- `/rename {displayname}` -> changes user displayname
- `/exit` -> exits program