# API Reference

## Header files

- [src/libs/buffer.h](#file-srclibsbufferh)
- [src/libs/cleanUpMaster.h](#file-srclibscleanupmasterh)
- [src/libs/ipk24protocol.h](#file-srclibsipk24protocolh)
- [src/libs/msgQueue.h](#file-srclibsmsgqueueh)
- [src/libs/networkCom.h](#file-srclibsnetworkcomh)
- [src/libs/programInterface.h](#file-srclibsprograminterfaceh)
- [src/libs/utils.h](#file-srclibsutilsh)
- [src/protocolReceiver.h](#file-srcprotocolreceiverh)
- [src/protocolSender.h](#file-srcprotocolsenderh)
- [tests/testUtils.h](#file-teststestutilsh)

## File src/libs/buffer.h

_Structures and declaration of functions for_ [_**Buffer**_](#struct-buffer)_._

**Author:**

Denis Fekete ([xfeket01@vutbr.cz](mailto:xfeket01@vutbr.cz))

[**Buffer**](#struct-buffer) is an structure for defining byte arrays (char arrays / string) with information about how many bytes has been allocated and how many has been used.



**Copyright:**

Copyright (c) 2024

## Structures and Types

| Type | Name |
| ---: | :--- |
| struct | [**Buffer**](#struct-buffer) <br>[_**Buffer**_](#struct-buffer)_ is an structure for defining byte arrays (char arrays / string) with information about how many bytes has been allocated and how many has been used._ |
| typedef struct [**Buffer**](#struct-buffer) | [**Buffer**](#typedef-buffer)  <br>[_**Buffer**_](#struct-buffer)_ is an structure for defining byte arrays (char arrays / string) with information about how many bytes has been allocated and how many has been used._ |

## Functions

| Type | Name |
| ---: | :--- |
|  void | [**bufferCopy**](#function-buffercopy) ([**Buffer**](#struct-buffer) \*dst, [**Buffer**](#struct-buffer) \*src) <br>_Copies contents from src buffer to dst._ |
|  void | [**bufferDestroy**](#function-bufferdestroy) ([**Buffer**](#struct-buffer) \*buffer) <br>_Destroys_ [_**Buffer**_](#struct-buffer)_ and frees memory._ |
|  void | [**bufferInit**](#function-bufferinit) ([**Buffer**](#struct-buffer) \*buffer) <br>_Sets default values to the buffer._ |
|  void | [**bufferPrint**](#function-bufferprint) ([**Buffer**](#struct-buffer) \*buffer, int useDebugPrint) <br>_Prints buffer characters byte by byte from start to used._ |
|  void | [**bufferResize**](#function-bufferresize) ([**Buffer**](#struct-buffer) \*buffer, size\_t newSize) <br>_Resizes buffer to new size, if buffer is not iniatilized (NULL) default value (INITIAL\_BUFFER\_SIZE) will be used instead to prevent allocation of small buffers._ |
|  size\_t | [**loadBufferFromStdin**](#function-loadbufferfromstdin) ([**Buffer**](#struct-buffer) \*buffer, bool \*eofDetected) <br>_Fills buffer with characters from stdin._ |

## Macros

| Type | Name |
| ---: | :--- |
| define  | [**INITIAL\_BUFFER\_SIZE**](#define-initial_buffer_size)  256<br> |

## Structures and Types Documentation

### struct `Buffer`

[_**Buffer**_](#struct-buffer)_ is an structure for defining byte arrays (char arrays / string) with information about how many bytes has been allocated and how many has been used._

Variables:

-  size\_t allocated  

-  char \* data  

-  size\_t used  

### typedef `Buffer`

[_**Buffer**_](#struct-buffer)_ is an structure for defining byte arrays (char arrays / string) with information about how many bytes has been allocated and how many has been used._
```c
typedef struct Buffer Buffer;
```


## Functions Documentation

### function `bufferCopy`

_Copies contents from src buffer to dst._
```c
void bufferCopy (
    Buffer *dst,
    Buffer *src
) 
```


**Parameters:**


* `dst` [**Buffer**](#struct-buffer) to which data will be copied
* `src` [**Buffer**](#struct-buffer) from which data will be copied
### function `bufferDestroy`

_Destroys_ [_**Buffer**_](#struct-buffer)_ and frees memory._
```c
void bufferDestroy (
    Buffer *buffer
) 
```


**Parameters:**


* `buffer`
### function `bufferInit`

_Sets default values to the buffer._
```c
void bufferInit (
    Buffer *buffer
) 
```


**Warning:**

Do not use on buffer that already has allocated memory



**Parameters:**


* `buffer` [**Buffer**](#struct-buffer) to be reseted
### function `bufferPrint`

_Prints buffer characters byte by byte from start to used._
```c
void bufferPrint (
    Buffer *buffer,
    int useDebugPrint
) 
```


**Parameters:**


* `buffer` Input buffer 
* `hex` If not 0 (false) prints hex values with white spaces between 
* `smartfilter` If not 0 (false) only alphanumeric chacters will be prited as chracaters and other chars will be printed as hex codes
### function `bufferResize`

_Resizes buffer to new size, if buffer is not iniatilized (NULL) default value (INITIAL\_BUFFER\_SIZE) will be used instead to prevent allocation of small buffers._
```c
void bufferResize (
    Buffer *buffer,
    size_t newSize
) 
```


**Parameters:**


* `buffer` [**Buffer**](#struct-buffer) to be resized
* `newSize` New size
### function `loadBufferFromStdin`

_Fills buffer with characters from stdin._
```c
size_t loadBufferFromStdin (
    Buffer *buffer,
    bool *eofDetected
) 
```


Fills buffer character by character until EOF is found. If buffer is running out of space, it will be resized



**Parameters:**


* `buffer` Pointer to the buffer. Can be inputed as NULL, however correct buffer size is required 
* `bufferSize` Pointer size of provided buffer

## Macros Documentation

### define `INITIAL_BUFFER_SIZE`

```c
#define INITIAL_BUFFER_SIZE 256
```


## File src/libs/cleanUpMaster.h

_Implementation of functions for Program Interface initializing and destroying, as well as function for handling SIGINT singals._

Declarations of functions for Program Interface initializing and destroying, as well as function for handling SIGINT singals.



**Author:**

Denis Fekete ([xfeket01@vutbr.cz](mailto:xfeket01@vutbr.cz))



**Copyright:**

Copyright (c) 2024


## Functions

| Type | Name |
| ---: | :--- |
|  void | [**programInterfaceDestroy**](#function-programinterfacedestroy) ([**ProgramInterface**](#struct-programinterface) \*pI) <br>_Destorys and frees allocated structures inside global program interface._ |
|  void | [**programInterfaceInit**](#function-programinterfaceinit) ([**ProgramInterface**](#struct-programinterface) \*pI) <br>_Initializes allocated structures in program interface._ |
|  void | [**sigintHandler**](#function-siginthandler) (int num) <br>_Function that handles SIGINT signals and correctly frees up all memory and closes add comminication._ |



## Functions Documentation

### function `programInterfaceDestroy`

_Destorys and frees allocated structures inside global program interface._
```c
void programInterfaceDestroy (
    ProgramInterface *pI
) 
```

### function `programInterfaceInit`

_Initializes allocated structures in program interface._
```c
void programInterfaceInit (
    ProgramInterface *pI
) 
```

### function `sigintHandler`

_Function that handles SIGINT signals and correctly frees up all memory and closes add comminication._
```c
void sigintHandler (
    int num
) 
```


**Parameters:**


* `num`


## File src/libs/ipk24protocol.h

_Declaration of functions and structures for working with IPK24CHAT Protocol such as assembling or disassembling protocols for both UDP and TCP variants. Also contains separating user input into an ByteBlocks for further processing._

**Author:**

Denis Fekete ([xfeket01@vutbr.cz](mailto:xfeket01@vutbr.cz))



**Copyright:**

Copyright (c) 2024

## Structures and Types

| Type | Name |
| ---: | :--- |
| struct | [**ProtocolBlocks**](#struct-protocolblocks) <br>_Sturcture for assembling and dissasembling protocols to make more readable what is getting store where with the use of unions._ |
| typedef struct [**ProtocolBlocks**](#struct-protocolblocks) | [**ProtocolBlocks**](#typedef-protocolblocks)  <br>_Sturcture for assembling and dissasembling protocols to make more readable what is getting store where with the use of unions._ |

## Functions

| Type | Name |
| ---: | :--- |
|  bool | [**assembleProtocolTCP**](#function-assembleprotocoltcp) ([**ProtocolBlocks**](#struct-protocolblocks) \*pBlocks, [**Buffer**](#struct-buffer) \*buffer, [**ProgramInterface**](#struct-programinterface) \*progInt) <br>_Assembles protocol from commands and command type into a buffer in TCP format._ |
|  bool | [**assembleProtocolUDP**](#function-assembleprotocoludp) ([**ProtocolBlocks**](#struct-protocolblocks) \*pBlocks, [**Buffer**](#struct-buffer) \*buffer, [**ProgramInterface**](#struct-programinterface) \*progInt) <br>_Assembles protocol from commands and command type into a buffer in UDP format._ |
|  bool | [**disassebleProtocolTCP**](#function-disassebleprotocoltcp) ([**Buffer**](#struct-buffer) \*buffer, [**ProtocolBlocks**](#struct-protocolblocks) \*pBlocks) <br>_Dissassembles protocol from_ [_**Buffer**_](#struct-buffer)_ into commands, msgType and msgId for TCP variant._ |
|  bool | [**disassebleProtocolUDP**](#function-disassebleprotocoludp) ([**Buffer**](#struct-buffer) \*buffer, [**ProtocolBlocks**](#struct-protocolblocks) \*pBlocks, uint16\_t \*msgId) <br>_Dissassembles protocol from_ [_**Buffer**_](#struct-buffer)_ into commands, msgType and msgId for UDP variant._ |
|  bool | [**userInputToCmds**](#function-userinputtocmds) ([**Buffer**](#struct-buffer) \*buffer, [**ProtocolBlocks**](#struct-protocolblocks) \*pBlocks, bool \*eofDetected, [**msg\_flags**](#typedef-msg_flags) \*flags) <br>_Takes input from user (client) from buffer and break it into an array of commands (ByteBlock)_ |


## Structures and Types Documentation

### struct `ProtocolBlocks`

_Sturcture for assembling and dissasembling protocols to make more readable what is getting store where with the use of unions._

Variables:

-  union ProtocolBlocks::@0 @1  

-  union ProtocolBlocks::@2 @3  

-  union ProtocolBlocks::@4 @5  

-  union ProtocolBlocks::@6 @7  

-  [**BytesBlock**](#struct-bytesblock) cmd_auth_displayname  

-  [**BytesBlock**](#struct-bytesblock) cmd_auth_secret  

-  [**BytesBlock**](#struct-bytesblock) cmd_auth_username  

-  [**BytesBlock**](#struct-bytesblock) cmd_command  

-  [**BytesBlock**](#struct-bytesblock) cmd_conf_highMsgID  

-  [**BytesBlock**](#struct-bytesblock) cmd_conf_lowMsgID  

-  [**BytesBlock**](#struct-bytesblock) cmd_err_MsgContents  

-  [**BytesBlock**](#struct-bytesblock) cmd_join_channelID  

-  [**BytesBlock**](#struct-bytesblock) cmd_msg_MsgContents  

-  [**BytesBlock**](#struct-bytesblock) cmd_rename_displayname  

-  [**BytesBlock**](#struct-bytesblock) first  

-  uint16\_t msgID  

-  [**BytesBlock**](#struct-bytesblock) msg_auth_displayname  

-  [**BytesBlock**](#struct-bytesblock) msg_auth_secret  

-  [**BytesBlock**](#struct-bytesblock) msg_auth_username  

-  [**BytesBlock**](#struct-bytesblock) msg_err_MsgContents  

-  [**BytesBlock**](#struct-bytesblock) msg_err_displayname  

-  [**BytesBlock**](#struct-bytesblock) msg_join_channelID  

-  [**BytesBlock**](#struct-bytesblock) msg_join_displayname  

-  [**BytesBlock**](#struct-bytesblock) msg_msg_MsgContents  

-  [**BytesBlock**](#struct-bytesblock) msg_msg_displayname  

-  [**BytesBlock**](#struct-bytesblock) msg_reply_MsgContents  

-  [**BytesBlock**](#struct-bytesblock) msg_reply_refMsgID  

-  [**BytesBlock**](#struct-bytesblock) msg_reply_result  

-  bool msg_reply_result_bool  

-  [**BytesBlock**](#struct-bytesblock) second  

-  [**BytesBlock**](#struct-bytesblock) third  

-  unsigned char type  

-  [**BytesBlock**](#struct-bytesblock) zeroth  

### typedef `ProtocolBlocks`

_Sturcture for assembling and dissasembling protocols to make more readable what is getting store where with the use of unions._
```c
typedef struct ProtocolBlocks ProtocolBlocks;
```


## Functions Documentation

### function `assembleProtocolTCP`

_Assembles protocol from commands and command type into a buffer in TCP format._
```c
bool assembleProtocolTCP (
    ProtocolBlocks *pBlocks,
    Buffer *buffer,
    ProgramInterface *progInt
) 
```


**Parameters:**


* `pBlocks` Separated commands and values from user input 
* `buffer` Output buffer to be trasmited to the server that don't have all informations provided by user at start 
* `progInt` Pointer to [**ProgramInterface**](#struct-programinterface)


**Returns:**

Returns true if buffer can be sended to the server
### function `assembleProtocolUDP`

_Assembles protocol from commands and command type into a buffer in UDP format._
```c
bool assembleProtocolUDP (
    ProtocolBlocks *pBlocks,
    Buffer *buffer,
    ProgramInterface *progInt
) 
```


**Parameters:**


* `pBlocks` Separated commands and values from user input 
* `buffer` Output buffer to be trasmited to the server that don't have all informations provided by user at start 
* `progInt` Pointer to [**ProgramInterface**](#struct-programinterface)


**Returns:**

Returns true if buffer can be sended to the server


Assembles protocol from commands and command type into a buffer in UDP format.



**Parameters:**


* `pBlocks` Separated commands and values from user input 
* `buffer` Output buffer to be trasmited to the server that don't have all informations provided by user at start 
* `progInt` Pointer to [**ProgramInterface**](#struct-programinterface)


**Returns:**

Returns true if buffer can be sended to the server
### function `disassebleProtocolTCP`

_Dissassembles protocol from_ [_**Buffer**_](#struct-buffer)_ into commands, msgType and msgId for TCP variant._
```c
bool disassebleProtocolTCP (
    Buffer *buffer,
    ProtocolBlocks *pBlocks
) 
```


**Parameters:**


* `buffer` Input buffer containing message 
* `pBlocks` Separated commands and values from user input 
* `msgId` Detected message ID
### function `disassebleProtocolUDP`

_Dissassembles protocol from_ [_**Buffer**_](#struct-buffer)_ into commands, msgType and msgId for UDP variant._
```c
bool disassebleProtocolUDP (
    Buffer *buffer,
    ProtocolBlocks *pBlocks,
    uint16_t *msgId
) 
```


**Parameters:**


* `buffer` Input buffer containing message 
* `pBlocks` Separated commands and values from user input 
* `msgId` Detected message ID
### function `userInputToCmds`

_Takes input from user (client) from buffer and break it into an array of commands (ByteBlock)_
```c
bool userInputToCmds (
    Buffer *buffer,
    ProtocolBlocks *pBlocks,
    bool *eofDetected,
    msg_flags *flags
) 
```


**Warning:**

Commands contain only pointers to the buffer, rewriting buffer will lead to unexpected behavior commands in commands



**Parameters:**


* `buffer` Input buffer containing command from user (client) 
* `commands` Array of commands where separated commands will be store 
* `eofDetected` Signals that end of file was detected 
* `flags` Flags that will be set to message 


**Returns:**

bool Returns whenever parameters are valid


## File src/libs/msgQueue.h

_Declaration of fucntions and structures of_ [_**MessageQueue**_](#struct-messagequeue)_ for storing and correctly working with multiple messages._

**Author:**

Denis Fekete ([xfeket01@vutbr.cz](mailto:xfeket01@vutbr.cz))



**Copyright:**

Copyright (c) 2024

## Structures and Types

| Type | Name |
| ---: | :--- |
| struct | [**Message**](#struct-message) <br>_Mesage in list containing_ [_**Buffer**_](#struct-buffer)_ with message contents, type of message, flag and pointer to the message behind this message._ |
| typedef struct [**Message**](#struct-message) | [**Message**](#typedef-message)  <br>_Mesage in list containing_ [_**Buffer**_](#struct-buffer)_ with message contents, type of message, flag and pointer to the message behind this message._ |
| enum  | [**MessageFlags**](#enum-messageflags)  <br>_Flags that can be added to the send messages._ |
| struct | [**MessageQueue**](#struct-messagequeue) <br>[_**MessageQueue**_](#struct-messagequeue)_ is priority FIFO (first in first out) queue containg_[_**Message**_](#struct-message)_ structures and mutex lock for protecting data from being access at multiple points in same time._ |
| typedef struct [**MessageQueue**](#struct-messagequeue) | [**MessageQueue**](#typedef-messagequeue)  <br>[_**MessageQueue**_](#struct-messagequeue)_ is priority FIFO (first in first out) queue containg_[_**Message**_](#struct-message)_ structures and mutex lock for protecting data from being access at multiple points in same time._ |
| enum  | [**MessageType**](#enum-messagetype)  <br>_Type of messages._ |
| typedef struct [**ProgramInterface**](#struct-programinterface) | [**ProgramInterface**](#typedef-programinterface)  <br> |
| typedef enum [**MessageFlags**](#enum-messageflags) | [**msg\_flags**](#typedef-msg_flags)  <br>_Flags that can be added to the send messages._ |
| typedef enum [**MessageType**](#enum-messagetype) | [**msg\_t**](#typedef-msg_t)  <br>_Type of messages._ |

## Functions

| Type | Name |
| ---: | :--- |
|  [**Message**](#struct-message) \* | [**createMessage**](#function-createmessage) ([**Buffer**](#struct-buffer) \*buffer, [**msg\_flags**](#typedef-msg_flags) msgFlags) <br>_Creates and initializes message and returns pointer to it._ |
|  void | [**queueAddMessage**](#function-queueaddmessage) ([**MessageQueue**](#struct-messagequeue) \*queue, [**Buffer**](#struct-buffer) \*buffer, [**msg\_flags**](#typedef-msg_flags) msgFlags, unsigned char cmdType) <br>_Adds new message to the queue at the end._ |
|  void | [**queueAddMessageOnlyID**](#function-queueaddmessageonlyid) ([**MessageQueue**](#struct-messagequeue) \*queue, [**Buffer**](#struct-buffer) \*buffer) <br>_Adds message to queue at the start. Added emssage won't contain buffer, only message ID._ |
|  void | [**queueAddMessagePriority**](#function-queueaddmessagepriority) ([**MessageQueue**](#struct-messagequeue) \*queue, [**Buffer**](#struct-buffer) \*buffer, [**msg\_flags**](#typedef-msg_flags) msgFlags) <br>_Adds new message to the queue at the start._ |
|  bool | [**queueContainsMessageId**](#function-queuecontainsmessageid) ([**MessageQueue**](#struct-messagequeue) \*queue, char highB, char lowB) <br>_Looks for message ID in queue, works only with message queues without buffers containing only msg ids._ |
|  void | [**queueDestroy**](#function-queuedestroy) ([**MessageQueue**](#struct-messagequeue) \*queue) <br>_Destroys_ [_**MessageQueue**_](#struct-messagequeue)_._ |
|  [**Message**](#struct-message) \* | [**queueGetMessage**](#function-queuegetmessage) ([**MessageQueue**](#struct-messagequeue) \*queue) <br>_Return pointer to the first message._ |
|  [**msg\_flags**](#typedef-msg_flags) | [**queueGetMessageFlags**](#function-queuegetmessageflags) ([**MessageQueue**](#struct-messagequeue) \*queue) <br>_Returns value of first message flags._ |
|  uint16\_t | [**queueGetMessageID**](#function-queuegetmessageid) ([**MessageQueue**](#struct-messagequeue) \*queue) <br>_Returns message ID of the first message from queue._ |
|  [**msg\_t**](#typedef-msg_t) | [**queueGetMessageMsgType**](#function-queuegetmessagemsgtype) ([**MessageQueue**](#struct-messagequeue) \*queue) <br>_Returns MessageType (msg\_t) of the first message in queue._ |
|  u\_int8\_t | [**queueGetSendedCounter**](#function-queuegetsendedcounter) ([**MessageQueue**](#struct-messagequeue) \*queue) <br>_Returns number of times this message was sended._ |
|  void | [**queueInit**](#function-queueinit) ([**MessageQueue**](#struct-messagequeue) \*queue) <br>_Initializes_ [_**MessageQueue**_](#struct-messagequeue)_ with default size (DEFAULT\_MESSAGE\_QUEUE\_SIZE)_ |
|  bool | [**queueIsEmpty**](#function-queueisempty) ([**MessageQueue**](#struct-messagequeue) \*queue) <br>_Check if_ [_**MessageQueue**_](#struct-messagequeue)_ is empty._ |
|  size\_t | [**queueLength**](#function-queuelength) ([**MessageQueue**](#struct-messagequeue) \*queue) <br>_Returns length of MesssageQueue._ |
|  void | [**queueLock**](#function-queuelock) ([**MessageQueue**](#struct-messagequeue) \*queue) <br>_Lock queue mutex._ |
|  void | [**queueMessageSended**](#function-queuemessagesended) ([**MessageQueue**](#struct-messagequeue) \*queue) <br>_Adds ONE to sended counter of first message._ |
|  void | [**queuePopAllMessages**](#function-queuepopallmessages) ([**MessageQueue**](#struct-messagequeue) \*queue) <br>_Deletes all messages in queue._ |
|  void | [**queuePopMessage**](#function-queuepopmessage) ([**MessageQueue**](#struct-messagequeue) \*queue) <br>_Deletes first message and moves queue forward._ |
|  void | [**queueSetMessageFlags**](#function-queuesetmessageflags) ([**MessageQueue**](#struct-messagequeue) \*queue, [**msg\_flags**](#typedef-msg_flags) newFlag) <br>_Sets flag of the first message in queue._ |
|  void | [**queueSetMessageID**](#function-queuesetmessageid) ([**MessageQueue**](#struct-messagequeue) \*queue, [**ProgramInterface**](#struct-programinterface) \*progInt) <br>_Sets message id of the first message based on program interface message counter._ |
|  void | [**queueUnlock**](#function-queueunlock) ([**MessageQueue**](#struct-messagequeue) \*queue) <br>_Unlock queue mutex._ |
|  [**msg\_t**](#typedef-msg_t) | [**uchar2msgType**](#function-uchar2msgtype) (unsigned char input) <br>_Converts integer into and MessageType._ |


## Structures and Types Documentation

### struct `Message`

_Mesage in list containing_ [_**Buffer**_](#struct-buffer)_ with message contents, type of message, flag and pointer to the message behind this message._

Variables:

-  union Message::@10 @11  

-  union Message::@8 @9  

-  struct [**Message**](#struct-message) \* behindMe  

-  [**Buffer**](#struct-buffer) \* buffer  

-  bool confirmed  

-  unsigned char highMsgId  

-  unsigned char lowMsgId  

-  [**msg\_flags**](#typedef-msg_flags) msgFlags  

-  u\_int8\_t sendCount  

-  unsigned char type  

### typedef `Message`

_Mesage in list containing_ [_**Buffer**_](#struct-buffer)_ with message contents, type of message, flag and pointer to the message behind this message._
```c
typedef struct Message Message;
```

### enum `MessageFlags`

_Flags that can be added to the send messages._
```c
enum MessageFlags {
    msg_flag_NONE,
    msg_flag_DO_NOT_RESEND,
    msg_flag_NOK_REPLY,
    msg_flag_AUTH,
    msg_flag_REJECTED,
    msg_flag_CONFIRMED,
    msg_flag_ERR,
    msg_flag_CONFIRM
};
```

### struct `MessageQueue`

[_**MessageQueue**_](#struct-messagequeue)_ is priority FIFO (first in first out) queue containg_[_**Message**_](#struct-message)_ structures and mutex lock for protecting data from being access at multiple points in same time._

Variables:

-  [**Message**](#struct-message) \* first  

-  [**Message**](#struct-message) \* last  

-  size\_t len  

-  pthread\_mutex\_t lock  

### typedef `MessageQueue`

[_**MessageQueue**_](#struct-messagequeue)_ is priority FIFO (first in first out) queue containg_[_**Message**_](#struct-message)_ structures and mutex lock for protecting data from being access at multiple points in same time._
```c
typedef struct MessageQueue MessageQueue;
```

### enum `MessageType`

_Type of messages._
```c
enum MessageType {
    msg_CONF = 0x00,
    msg_REPLY = 0x01,
    msg_AUTH = 0x02,
    msg_JOIN = 0x03,
    msg_MSG = 0x04,
    msg_ERR = 0xFE,
    msg_BYE = 0xFF,
    msg_UNKNOWN = 0xAA
};
```

### typedef `ProgramInterface`

```c
typedef struct ProgramInterface ProgramInterface;
```

### typedef `msg_flags`

_Flags that can be added to the send messages._
```c
typedef enum MessageFlags msg_flags;
```

### typedef `msg_t`

_Type of messages._
```c
typedef enum MessageType msg_t;
```


## Functions Documentation

### function `createMessage`

_Creates and initializes message and returns pointer to it._
```c
Message * createMessage (
    Buffer *buffer,
    msg_flags msgFlags
) 
```


**Parameters:**


* `buffer` Contents of buffer that will be copied into message 
* `msgFlags` Flags that will be set 


**Returns:**

Message\* Pointer to new allocated message
### function `queueAddMessage`

_Adds new message to the queue at the end._
```c
void queueAddMessage (
    MessageQueue *queue,
    Buffer *buffer,
    msg_flags msgFlags,
    unsigned char cmdType
) 
```


**Parameters:**


* `queue` [**MessageQueue**](#struct-messagequeue) to which will the new message be added
* `buffer` is and input buffer from which the new message will be created 
* `cmdType` type of command to be set to the message
### function `queueAddMessageOnlyID`

_Adds message to queue at the start. Added emssage won't contain buffer, only message ID._
```c
void queueAddMessageOnlyID (
    MessageQueue *queue,
    Buffer *buffer
) 
```


**Parameters:**


* `queue` Pointer to the queue 
* `buffer` [**Buffer**](#struct-buffer) where message is stored
### function `queueAddMessagePriority`

_Adds new message to the queue at the start._
```c
void queueAddMessagePriority (
    MessageQueue *queue,
    Buffer *buffer,
    msg_flags msgFlags
) 
```


**Parameters:**


* `queue` [**MessageQueue**](#struct-messagequeue) to which will the new message be added
* `buffer` is and input buffer from which the new message will be created
### function `queueContainsMessageId`

_Looks for message ID in queue, works only with message queues without buffers containing only msg ids._
```c
bool queueContainsMessageId (
    MessageQueue *queue,
    char highB,
    char lowB
) 
```


**Parameters:**


* `queue` Pointer to the queue 
* `highB` Higer byte to compare to 
* `lowB` Lower byte to compare to 


**Returns:**

true [**Message**](#struct-message) with this ID was found



**Returns:**

false [**Message**](#struct-message) with this ID was not found
### function `queueDestroy`

_Destroys_ [_**MessageQueue**_](#struct-messagequeue)_._
```c
void queueDestroy (
    MessageQueue *queue
) 
```


**Parameters:**


* `queue` queue to be destroyed
### function `queueGetMessage`

_Return pointer to the first message._
```c
Message * queueGetMessage (
    MessageQueue *queue
) 
```


**Parameters:**


* `queue` Queue from which the message will be returned 


**Returns:**

Message\*
### function `queueGetMessageFlags`

_Returns value of first message flags._
```c
msg_flags queueGetMessageFlags (
    MessageQueue *queue
) 
```


**Parameters:**


* `queue` Queue from which the first message's flags will be returned 


**Returns:**

msg\_flags flags to be returned
### function `queueGetMessageID`

_Returns message ID of the first message from queue._
```c
uint16_t queueGetMessageID (
    MessageQueue *queue
) 
```


**Parameters:**


* `queue` Pointer to queue 


**Returns:**

uint16\_t
### function `queueGetMessageMsgType`

_Returns MessageType (msg\_t) of the first message in queue._
```c
msg_t queueGetMessageMsgType (
    MessageQueue *queue
) 
```


**Parameters:**


* `queue` Pointer to the queue 


**Returns:**

msg\_t Detected value



**Parameters:**


* `queue` Pointer to the queue 


**Returns:**

cmd\_t Detected value
### function `queueGetSendedCounter`

_Returns number of times this message was sended._
```c
u_int8_t queueGetSendedCounter (
    MessageQueue *queue
) 
```


**Parameters:**


* `queue` queue from which first message will be checked 


**Returns:**

u\_int8\_t number of times first message in queue was sended
### function `queueInit`

_Initializes_ [_**MessageQueue**_](#struct-messagequeue)_ with default size (DEFAULT\_MESSAGE\_QUEUE\_SIZE)_
```c
void queueInit (
    MessageQueue *queue
) 
```


**Parameters:**


* `queue` [**MessageQueue**](#struct-messagequeue) to be initalized
### function `queueIsEmpty`

_Check if_ [_**MessageQueue**_](#struct-messagequeue)_ is empty._
```c
bool queueIsEmpty (
    MessageQueue *queue
) 
```


**Parameters:**


* `queue` Queue to be checked 


**Returns:**

true if queue is empty 



**Returns:**

false if queue is not empty
### function `queueLength`

_Returns length of MesssageQueue._
```c
size_t queueLength (
    MessageQueue *queue
) 
```


**Parameters:**


* `queue` queue to be checked 


**Returns:**

size\_t Number of Messages in queue
### function `queueLock`

_Lock queue mutex._
```c
void queueLock (
    MessageQueue *queue
) 
```


**Parameters:**


* `queue` Queue to be unlocked
### function `queueMessageSended`

_Adds ONE to sended counter of first message._
```c
void queueMessageSended (
    MessageQueue *queue
) 
```


**Parameters:**


* `queue` queue to which first message counter will be incremented
### function `queuePopAllMessages`

_Deletes all messages in queue._
```c
void queuePopAllMessages (
    MessageQueue *queue
) 
```


**Parameters:**


* `queue` Queue from which will the messages be deleted
### function `queuePopMessage`

_Deletes first message and moves queue forward._
```c
void queuePopMessage (
    MessageQueue *queue
) 
```


**Parameters:**


* `queue` Queue from which will be the message deleted
### function `queueSetMessageFlags`

_Sets flag of the first message in queue._
```c
void queueSetMessageFlags (
    MessageQueue *queue,
    msg_flags newFlag
) 
```


**Parameters:**


* `queue` Queue from which the first message's flags will be changed 
* `newFlag` New message of type msg\_flags
### function `queueSetMessageID`

_Sets message id of the first message based on program interface message counter._
```c
void queueSetMessageID (
    MessageQueue *queue,
    ProgramInterface *progInt
) 
```


**Parameters:**


* `queue` Pointer to queue 
* `progInt` Pointer to [**ProgramInterface**](#struct-programinterface) based that holds correct message counter
### function `queueUnlock`

_Unlock queue mutex._
```c
void queueUnlock (
    MessageQueue *queue
) 
```


**Parameters:**


* `queue` Queue to be unlocked
### function `uchar2msgType`

_Converts integer into and MessageType._
```c
msg_t uchar2msgType (
    unsigned char input
) 
```


**Parameters:**


* `input` Input integer 


**Returns:**

msg\_t MessageType to be returned


## File src/libs/networkCom.h

_Declaration of functions and structures for network connections._

**Author:**

Denis Fekete ([xfeket01@vutbr.cz](mailto:xfeket01@vutbr.cz))



**Copyright:**

Copyright (c) 2024

## Structures and Types

| Type | Name |
| ---: | :--- |
| struct | [**NetworkConfig**](#struct-networkconfig) <br>_Structure holding current network configuration of program._ |
| typedef struct [**NetworkConfig**](#struct-networkconfig) | [**NetworkConfig**](#typedef-networkconfig)  <br>_Structure holding current network configuration of program._ |
| enum  | [**Protocols**](#enum-protocols)  <br>_Types of protocol that can be used by application._ |
| typedef enum [**Protocols**](#enum-protocols) | [**prot\_t**](#typedef-prot_t)  <br>_Types of protocol that can be used by application._ |

## Functions

| Type | Name |
| ---: | :--- |
|  void | [**defaultNetworkConfig**](#function-defaultnetworkconfig) ([**NetworkConfig**](#struct-networkconfig) \*config) <br>_Sets default values for_ [_**NetworkConfig**_](#struct-networkconfig)_._ |
|  struct sockaddr\_in | [**findServer**](#function-findserver) (const char \*serverHostname, uint16\_t serverPort) <br>_Finds server and returns it socket adress._ |
|  int | [**getSocket**](#function-getsocket) ([**prot\_t**](#typedef-prot_t) protocol) <br>_Get the Socket id._ |

## Macros

| Type | Name |
| ---: | :--- |
| define  | [**PORT\_NUMBER**](#define-port_number)  4567<br> |

## Structures and Types Documentation

### struct `NetworkConfig`

_Structure holding current network configuration of program._

Variables:

-  int openedSocket  

-  uint16\_t portNumber  

-  [**prot\_t**](#typedef-prot_t) protocol  

-  struct sockaddr \* serverAddress  

-  unsigned serverAddressSize  

-  uint8\_t udpMaxRetries  

-  uint16\_t udpTimeout  

### typedef `NetworkConfig`

_Structure holding current network configuration of program._
```c
typedef struct NetworkConfig NetworkConfig;
```

### enum `Protocols`

_Types of protocol that can be used by application._
```c
enum Protocols {
    prot_ERR =-50,
    prot_UDP =50,
    prot_TCP =100
};
```

### typedef `prot_t`

_Types of protocol that can be used by application._
```c
typedef enum Protocols prot_t;
```


## Functions Documentation

### function `defaultNetworkConfig`

_Sets default values for_ [_**NetworkConfig**_](#struct-networkconfig)_._
```c
void defaultNetworkConfig (
    NetworkConfig *config
) 
```


**Parameters:**


* `config` pointer to the netword config
### function `findServer`

_Finds server and returns it socket adress._
```c
struct sockaddr_in findServer (
    const char *serverHostname,
    uint16_t serverPort
) 
```


**Parameters:**


* `serverHostname` String containing server hostname 
* `serverPort` Port of server 


**Returns:**

struct sockaddr\_in Socket adress that was created
### function `getSocket`

_Get the Socket id._
```c
int getSocket (
    prot_t protocol
) 
```


**Parameters:**


* `protocol` Protocol that will be used to open socket (UDP or TCP) 


**Returns:**

int Socket id

## Macros Documentation

### define `PORT_NUMBER`

```c
#define PORT_NUMBER 4567
```


## File src/libs/programInterface.h

_Declaration of functions and sturctures for Program Interface. Program Interace is structure holding all informatio needed for communication between multiple threads and modules of program._

**Author:**

Denis Fekete ([xfeket01@vutbr.cz](mailto:xfeket01@vutbr.cz))



**Copyright:**

Copyright (c) 2024

## Structures and Types

| Type | Name |
| ---: | :--- |
| struct | [**CleanUp**](#struct-cleanup) <br>_Structure holding other structures that could be local in modules but needs to be cleaned up._ |
| typedef struct [**CleanUp**](#struct-cleanup) | [**CleanUp**](#typedef-cleanup)  <br>_Structure holding other structures that could be local in modules but needs to be cleaned up._ |
| struct | [**CommunicationDetails**](#struct-communicationdetails) <br>_Structure holding basic communication details._ |
| typedef struct [**CommunicationDetails**](#struct-communicationdetails) | [**CommunicationDetails**](#typedef-communicationdetails)  <br>_Structure holding basic communication details._ |
| struct | [**ProgramInterface**](#struct-programinterface) <br>_Program Interace is structure holding information that is shared between program, contains resources meant for multi-threaded synchronization, sharing data, holding data about current state of program etc... Also hold all dynamically allocated data in program for CleanUpMaster to free in case of SIGINT._ |
| typedef struct [**ProgramInterface**](#struct-programinterface) | [**ProgramInterface**](#typedef-programinterface)  <br>_Program Interace is structure holding information that is shared between program, contains resources meant for multi-threaded synchronization, sharing data, holding data about current state of program etc... Also hold all dynamically allocated data in program for CleanUpMaster to free in case of SIGINT._ |
| struct | [**ThreadCommunication**](#struct-threadcommunication) <br>_Structure holding structures and variables for communication between threads._ |
| typedef struct [**ThreadCommunication**](#struct-threadcommunication) | [**ThreadCommunication**](#typedef-threadcommunication)  <br>_Structure holding structures and variables for communication between threads._ |

## Functions

| Type | Name |
| ---: | :--- |
|  [**fsm\_t**](#typedef-fsm_t) | [**getProgramState**](#function-getprogramstate) ([**ProgramInterface**](#struct-programinterface) \*progInt) <br>_Returns program state with thread protecion using mutex._ |
|  void | [**printCliHelpMenu**](#function-printclihelpmenu) (const char \*executableName) <br>_Prints help menu when user inputs /help command._ |
|  void | [**printUserHelpMenu**](#function-printuserhelpmenu) ([**ProgramInterface**](#struct-programinterface) \*progInt) <br>_Prints user help menu in running client._ |
|  void | [**setProgramState**](#function-setprogramstate) ([**ProgramInterface**](#struct-programinterface) \*progInt, [**fsm\_t**](#typedef-fsm_t) newState) <br>_Changes program state to new state with thread protecion using mutex._ |


## Structures and Types Documentation

### struct `CleanUp`

_Structure holding other structures that could be local in modules but needs to be cleaned up._

Variables:

-  [**Buffer**](#struct-buffer) clientInput  

-  struct [**MessageQueue**](#struct-messagequeue) \* confirmedMessages  

-  [**Buffer**](#struct-buffer) protocolToSendedByMain  

-  [**Buffer**](#struct-buffer) protocolToSendedByReceiver  

-  [**Buffer**](#struct-buffer) serverResponse  

### typedef `CleanUp`

_Structure holding other structures that could be local in modules but needs to be cleaned up._
```c
typedef struct CleanUp CleanUp;
```

### struct `CommunicationDetails`

_Structure holding basic communication details._

displayname - identity of client that is send to server as channelID - currect channel that client is connected to msgCounter - counter of send messages

Variables:

-  [**Buffer**](#struct-buffer) channelID  

-  [**Buffer**](#struct-buffer) displayName  

-  uint16\_t msgCounter  

### typedef `CommunicationDetails`

_Structure holding basic communication details._
```c
typedef struct CommunicationDetails CommunicationDetails;
```


displayname - identity of client that is send to server as channelID - currect channel that client is connected to msgCounter - counter of send messages
### struct `ProgramInterface`

_Program Interace is structure holding information that is shared between program, contains resources meant for multi-threaded synchronization, sharing data, holding data about current state of program etc... Also hold all dynamically allocated data in program for CleanUpMaster to free in case of SIGINT._

Variables:

-  [**CleanUp**](#struct-cleanup) \* cleanUp  

-  [**CommunicationDetails**](#struct-communicationdetails) \* comDetails  

-  struct [**NetworkConfig**](#struct-networkconfig) \* netConfig  

-  [**ThreadCommunication**](#struct-threadcommunication) \* threads  

### typedef `ProgramInterface`

_Program Interace is structure holding information that is shared between program, contains resources meant for multi-threaded synchronization, sharing data, holding data about current state of program etc... Also hold all dynamically allocated data in program for CleanUpMaster to free in case of SIGINT._
```c
typedef struct ProgramInterface ProgramInterface;
```

### struct `ThreadCommunication`

_Structure holding structures and variables for communication between threads._

Variables:

-  pthread\_mutex\_t \* fsmMutex  

-  [**fsm\_t**](#typedef-fsm_t) fsmState  

-  pthread\_cond\_t \* mainCond  

-  pthread\_mutex\_t \* mainMutex  

-  pthread\_cond\_t \* rec2SenderCond  

-  pthread\_mutex\_t \* rec2SenderMutex  

-  pthread\_cond\_t \* senderEmptyQueueCond  

-  pthread\_mutex\_t \* senderEmptyQueueMutex  

-  struct [**MessageQueue**](#struct-messagequeue) \* sendingQueue  

-  pthread\_mutex\_t \* stdoutMutex  

### typedef `ThreadCommunication`

_Structure holding structures and variables for communication between threads._
```c
typedef struct ThreadCommunication ThreadCommunication;
```


## Functions Documentation

### function `getProgramState`

_Returns program state with thread protecion using mutex._
```c
fsm_t getProgramState (
    ProgramInterface *progInt
) 
```

### function `printCliHelpMenu`

_Prints help menu when user inputs /help command._
```c
void printCliHelpMenu (
    const char *executableName
) 
```

### function `printUserHelpMenu`

_Prints user help menu in running client._
```c
void printUserHelpMenu (
    ProgramInterface *progInt
) 
```


**Parameters:**


* `progInt` Pointer to [**ProgramInterface**](#struct-programinterface) for thread-safe printing
### function `setProgramState`

_Changes program state to new state with thread protecion using mutex._
```c
void setProgramState (
    ProgramInterface *progInt,
    fsm_t newState
) 
```


**Parameters:**


* `newState` New state to be set


## File src/libs/utils.h

_Declaration of functions and structures for common/basic tasks in program. Also defines Fininite State Machine state and Command Types._

**Author:**

Denis Fekete ([xfeket01@vutbr.cz](mailto:xfeket01@vutbr.cz))



**Copyright:**

Copyright (c) 2024

## Structures and Types

| Type | Name |
| ---: | :--- |
| struct | [**BytesBlock**](#struct-bytesblock) <br>_Points at the start of the bytes block (array) with len length._ |
| typedef struct [**BytesBlock**](#struct-bytesblock) | [**BytesBlock**](#typedef-bytesblock)  <br>_Points at the start of the bytes block (array) with len length._ |
| enum  | [**CommandType**](#enum-commandtype)  <br>_Enum for types of different commands that can be proccessed by program._ |
| enum  | [**FSM**](#enum-fsm)  <br>_Enum for FSM state of program._ |
| typedef enum [**CommandType**](#enum-commandtype) | [**cmd\_t**](#typedef-cmd_t)  <br>_Enum for types of different commands that can be proccessed by program._ |
| typedef enum [**FSM**](#enum-fsm) | [**fsm\_t**](#typedef-fsm_t)  <br>_Enum for FSM state of program._ |

## Functions

| Type | Name |
| ---: | :--- |
|  void | [**breakU16IntToBytes**](#function-breaku16inttobytes) (char \*high, char \*low, uint16\_t msgCounter) <br>_Converts 16bit message id into an two unsinged chars._ |
|  uint16\_t | [**convert2BytesToU16Int**](#function-convert2bytestou16int) (char high, char low) <br>_Converts bwo bytes from input char array into 16bit usigned integer._ |
|  int | [**errHandling**](#function-errhandling) (const char \*msg, int errorCode) <br>_Prints msg and exits program with errorCode._ |
|  long | [**findBlankCharInString**](#function-findblankcharinstring) (char \*string, size\_t len) <br>_Finds first blank character (spaces ' ' and tabulators '\t') in string and returns index of last character before blank character._ |
|  long | [**findNewLineInString**](#function-findnewlineinstring) (char \*string, size\_t len) <br>_Finds new line character in string._ |
|  long | [**findZeroInString**](#function-findzeroinstring) (char \*string, size\_t len) <br>_Returns index of last character before '\0' character in string._ |
|  bool | [**getWord**](#function-getword) ([**BytesBlock**](#struct-bytesblock) \*block, char \*startOfLastWord, size\_t bufferSize) <br>_Returns word from string. Look from first character until blank character is found._ |
|  int | [**isEndingCharacter**](#function-isendingcharacter) (char input) <br>_Returns true if character is considered as an ending character for command._ |
|  long | [**skipBlankCharsInString**](#function-skipblankcharsinstring) (char \*string, size\_t len) <br>_Skips blank character until a first non empty character is found._ |
|  void | [**stringReplace**](#function-stringreplace) (char \*dst, char \*src, size\_t len) <br>_Replaces bytes in dst with bytes from src up to len lenght._ |
|  [**cmd\_t**](#typedef-cmd_t) | [**uchar2CommandType**](#function-uchar2commandtype) (unsigned char input) <br>_Converts unsigned char into an CommandType._ |

## Macros

| Type | Name |
| ---: | :--- |
| define  | [**END\_VARIANTS**](#define-end_variants)  }<br> |
| define  | [**TCP\_VARIANT**](#define-tcp_variant)  } else if(progInt-&gt;netConfig-&gt;protocol == prot\_TCP) {<br> |
| define  | [**UDP\_VARIANT**](#define-udp_variant)  if(progInt-&gt;netConfig-&gt;protocol == prot\_UDP) {<br> |
| define  | [**debugPrint**](#define-debugprint) (...) ;<br> |
| define  | [**debugPrintSeparator**](#define-debugprintseparator) (fs) ;<br> |
| define  | [**safePrintStderr**](#define-safeprintstderr) (...) <br> |
| define  | [**safePrintStdout**](#define-safeprintstdout) (...) <br>_Macro for safe printing using "global" stdoutMutex._ |

## Structures and Types Documentation

### struct `BytesBlock`

_Points at the start of the bytes block (array) with len length._

**Note:**

Byte block in this context does not own the memory, it is just a pointer with length

Variables:

-  size\_t len  

-  char \* start  

### typedef `BytesBlock`

_Points at the start of the bytes block (array) with len length._
```c
typedef struct BytesBlock BytesBlock;
```


**Note:**

Byte block in this context does not own the memory, it is just a pointer with length
### enum `CommandType`

_Enum for types of different commands that can be proccessed by program._
```c
enum CommandType {
    cmd_AUTH,
    cmd_JOIN,
    cmd_RENAME,
    cmd_HELP,
    cmd_CONF,
    cmd_MSG,
    cmd_ERR,
    cmd_EXIT,
    cmd_NONE,
    cmd_CONVERSION_ERR
};
```

### enum `FSM`

_Enum for FSM state of program._
```c
enum FSM {
    fsm_START,
    fsm_AUTH_W82_BE_SENDED,
    fsm_AUTH_SENDED,
    fsm_W84_REPLY,
    fsm_W84_REPLY_CONF,
    fsm_OPEN,
    fsm_JOIN_ATEMPT,
    fsm_EMPTY_Q_BYE,
    fsm_BYE_RECV,
    fsm_ERR,
    fsm_SIGINT_BYE,
    fsm_END
};
```

### typedef `cmd_t`

_Enum for types of different commands that can be proccessed by program._
```c
typedef enum CommandType cmd_t;
```

### typedef `fsm_t`

_Enum for FSM state of program._
```c
typedef enum FSM fsm_t;
```


## Functions Documentation

### function `breakU16IntToBytes`

_Converts 16bit message id into an two unsinged chars._
```c
void breakU16IntToBytes (
    char *high,
    char *low,
    uint16_t msgCounter
) 
```


**Parameters:**


* `high` Output pointer to unsigned char, upper/higher half of number 
* `low` Output pointer to unsigned char, lower half of number 
* `msgCounter` Input number to be separated
### function `convert2BytesToU16Int`

_Converts bwo bytes from input char array into 16bit usigned integer._
```c
uint16_t convert2BytesToU16Int (
    char high,
    char low
) 
```


**Parameters:**


* `high` Higher byte 
* `low` Lower byte 


**Returns:**

uint16\_t
### function `errHandling`

_Prints msg and exits program with errorCode._
```c
int errHandling (
    const char *msg,
    int errorCode
) 
```


**Parameters:**


* `msg` [**Message**](#struct-message) to be printed
* `errorCode` Error code that will be used as exit code 


**Returns:**

int Returns 0 (for anti-compiler errors)
### function `findBlankCharInString`

_Finds first blank character (spaces ' ' and tabulators '\t') in string and returns index of last character before blank character._
```c
long findBlankCharInString (
    char *string,
    size_t len
) 
```


**Parameters:**


* `string` Input string to be searched in 
* `len` Length of string that cannot be exceeded 


**Returns:**

long Index in string
### function `findNewLineInString`

_Finds new line character in string._
```c
long findNewLineInString (
    char *string,
    size_t len
) 
```


**Parameters:**


* `string` Input string 
* `len` Maximum length 


**Returns:**

long Index in newline was found
### function `findZeroInString`

_Returns index of last character before '\0' character in string._
```c
long findZeroInString (
    char *string,
    size_t len
) 
```


**Parameters:**


* `string` Pointer to the string 
* `len` Length of the string 


**Returns:**

long Index of last character before \0
### function `getWord`

_Returns word from string. Look from first character until blank character is found._
```c
bool getWord (
    BytesBlock *block,
    char *startOfLastWord,
    size_t bufferSize
) 
```


**Parameters:**


* `block` [**BytesBlock**](#struct-bytesblock) to which should the result be saved
* `startOfLastWord` Pointer to the start of the last word 
* `bufferSize` Maximum size of buffer 


**Returns:**

true Success 



**Returns:**

false Failed
### function `isEndingCharacter`

_Returns true if character is considered as an ending character for command._
```c
int isEndingCharacter (
    char input
) 
```


**Parameters:**


* `input` Input character 


**Returns:**

int If yes 1 (True), 0 (False)
### function `skipBlankCharsInString`

_Skips blank character until a first non empty character is found._
```c
long skipBlankCharsInString (
    char *string,
    size_t len
) 
```


**Parameters:**


* `string` String to be searched in 
* `len` Length of string that cannot be 


**Returns:**

long
### function `stringReplace`

_Replaces bytes in dst with bytes from src up to len lenght._
```c
void stringReplace (
    char *dst,
    char *src,
    size_t len
) 
```


**Parameters:**


* `dst` Destinatin byte array 
* `src` Source byte arry 
* `len` Number of bytes to replace
### function `uchar2CommandType`

_Converts unsigned char into an CommandType._
```c
cmd_t uchar2CommandType (
    unsigned char input
) 
```


**Parameters:**


* `input` Input unsigned char 


**Returns:**

cmd\_t Returned value in cmd\_t

## Macros Documentation

### define `END_VARIANTS`

```c
#define END_VARIANTS }
```

### define `TCP_VARIANT`

```c
#define TCP_VARIANT } else if(progInt->netConfig->protocol == prot_TCP) {
```

### define `UDP_VARIANT`

```c
#define UDP_VARIANT if(progInt->netConfig->protocol == prot_UDP) {
```

### define `debugPrint`

```c
#define debugPrint (
    ...
) ;
```

### define `debugPrintSeparator`

```c
#define debugPrintSeparator (
    fs
) ;
```

### define `safePrintStderr`

```c
#define safePrintStderr (
    ...
) pthread_mutex_lock(progInt->threads->stdoutMutex);  \
    fprintf(stderr, __VA_ARGS__);                      \
    fflush(stderr);                                     \
    pthread_mutex_unlock(progInt->threads->stdoutMutex);
```

### define `safePrintStdout`

_Macro for safe printing using "global" stdoutMutex._
```c
#define safePrintStdout (
    ...
) pthread_mutex_lock(progInt->threads->stdoutMutex);  \
    printf(__VA_ARGS__);                                \
    fflush(stdout);                                     \
    pthread_mutex_unlock(progInt->threads->stdoutMutex);
```


Uses fflush after message has been written

## File src/protocolReceiver.h

_Header file for Protocol Receiver that receives messages from server and changes program state accordingly._

**Author:**

Denis Fekete ([xfeket01@vutbr.cz](mailto:xfeket01@vutbr.cz))


This module is meant to work in separated thread



**Copyright:**

Copyright (c) 2024


## Functions

| Type | Name |
| ---: | :--- |
|  void \* | [**protocolReceiver**](#function-protocolreceiver) (void \*vargp) <br>_Initializes protocol receiving functionality._ |
|  void | [**sendConfirm**](#function-sendconfirm) ([**Buffer**](#struct-buffer) \*serverResponse, [**Buffer**](#struct-buffer) \*receiverSendMsgs, [**ProgramInterface**](#struct-programinterface) \*progInt, [**msg\_flags**](#typedef-msg_flags) flags) <br>_Create confirm protocol._ |
|  void | [**sendError**](#function-senderror) ([**Buffer**](#struct-buffer) \*serverResponse, [**Buffer**](#struct-buffer) \*receiverSendMsgs, [**ProgramInterface**](#struct-programinterface) \*progInt, const char \*message) <br>_Create err protocol._ |



## Functions Documentation

### function `protocolReceiver`

_Initializes protocol receiving functionality._
```c
void * protocolReceiver (
    void *vargp
) 
```


**Parameters:**


* `vargp` arguments 


**Returns:**

void\*
### function `sendConfirm`

_Create confirm protocol._
```c
void sendConfirm (
    Buffer *serverResponse,
    Buffer *receiverSendMsgs,
    ProgramInterface *progInt,
    msg_flags flags
) 
```


**Parameters:**


* `serverResponse` [**Buffer**](#struct-buffer) from which will referenceID be taken
* `receiverSendMsgs` [**Buffer**](#struct-buffer) to which should message be stored
* `progInt` Pointer to Program Interface 
* `flags` Flags to be added to the message
### function `sendError`

_Create err protocol._
```c
void sendError (
    Buffer *serverResponse,
    Buffer *receiverSendMsgs,
    ProgramInterface *progInt,
    const char *message
) 
```


**Parameters:**


* `serverResponse` [**Buffer**](#struct-buffer) from which will referenceID be taken
* `receiverSendMsgs` [**Buffer**](#struct-buffer) to which should message be stored
* `progInt` Pointer to Program Interface 
* `message` [**Message**](#struct-message) to be sended to the server


## File src/protocolSender.h

_Header file for Protocol Sender that sends messages to server and changes program state accordingly._

**Author:**

Denis Fekete ([xfeket01@vutbr.cz](mailto:xfeket01@vutbr.cz))


This module is meant to work in separated thread.



**Copyright:**

Copyright (c) 2024


## Functions

| Type | Name |
| ---: | :--- |
|  void \* | [**protocolSender**](#function-protocolsender) (void \*vargp) <br>_Initializes protocol sending functionality._ |



## Functions Documentation

### function `protocolSender`

_Initializes protocol sending functionality._
```c
void * protocolSender (
    void *vargp
) 
```


**Parameters:**


* `vargp` arguments 


**Returns:**

void\*


## File tests/testUtils.h







## Macros

| Type | Name |
| ---: | :--- |
| define  | [**ASSERT**](#define-assert) (cond) if(!(cond)) {<br> |
| define  | [**ON\_FAIL**](#define-on_fail) (message) <br> |
| define  | [**TEST\_NAME**](#define-test_name) (name) const char\* testname = name;<br> |



## Macros Documentation

### define `ASSERT`

```c
#define ASSERT (
    cond
) if(!(cond)) {
```

### define `ON_FAIL`

```c
#define ON_FAIL (
    message
) fprintf(stderr, "%s:", testname);      \
    fprintf(stderr, message);   \
    fprintf(stderr, "\n");      \
    return 1;                   \
    }
```

### define `TEST_NAME`

```c
#define TEST_NAME (
    name
) const char* testname = name;
```


