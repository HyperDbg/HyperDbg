/**
 * @file forwarding.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers for event source forwarding
 * @details
 * @version 0.1
 * @date 2020-11-16
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////
//           Forwarding Types           //
//////////////////////////////////////////

/**
 * @brief maximum characters for event forwarding source names
 *
 */
typedef void (*hyperdbg_event_forwarding_t)(const char *, unsigned int);

//////////////////////////////////////////
//     Output Source Forwarding         //
//////////////////////////////////////////

/**
 * @brief maximum characters for event forwarding source names
 *
 */
#define MAXIMUM_CHARACTERS_FOR_EVENT_FORWARDING_NAME 50

/**
 * @brief event forwarding type
 *
 */
typedef enum _DEBUGGER_EVENT_FORWARDING_TYPE
{
    EVENT_FORWARDING_NAMEDPIPE,
    EVENT_FORWARDING_FILE,
    EVENT_FORWARDING_TCP,
    EVENT_FORWARDING_MODULE,

} DEBUGGER_EVENT_FORWARDING_TYPE;

/**
 * @brief event forwarding states
 *
 */
typedef enum _DEBUGGER_EVENT_FORWARDING_STATE
{
    EVENT_FORWARDING_STATE_NOT_OPENED,
    EVENT_FORWARDING_STATE_OPENED,
    EVENT_FORWARDING_CLOSED,

} DEBUGGER_EVENT_FORWARDING_STATE;

/**
 * @brief output source status
 *
 * @details this enum is used as the result returned from
 * the functions that work with opening and closing sources
 */
typedef enum _DEBUGGER_OUTPUT_SOURCE_STATUS
{
    DEBUGGER_OUTPUT_SOURCE_STATUS_SUCCESSFULLY_OPENED,
    DEBUGGER_OUTPUT_SOURCE_STATUS_SUCCESSFULLY_CLOSED,
    DEBUGGER_OUTPUT_SOURCE_STATUS_ALREADY_OPENED,
    DEBUGGER_OUTPUT_SOURCE_STATUS_ALREADY_CLOSED,
    DEBUGGER_OUTPUT_SOURCE_STATUS_UNKNOWN_ERROR,

} DEBUGGER_OUTPUT_SOURCE_STATUS;

/**
 * @brief structures hold the detail of event forwarding
 *
 */
typedef struct _DEBUGGER_EVENT_FORWARDING
{
    DEBUGGER_EVENT_FORWARDING_TYPE  Type;
    DEBUGGER_EVENT_FORWARDING_STATE State;
    VOID *                          Handle;
    SOCKET                          Socket;
    HMODULE                         Module;
    UINT64                          OutputUniqueTag;
    LIST_ENTRY
    OutputSourcesList; // Linked-list of output sources list
    CHAR Name[MAXIMUM_CHARACTERS_FOR_EVENT_FORWARDING_NAME];

} DEBUGGER_EVENT_FORWARDING, *PDEBUGGER_EVENT_FORWARDING;

//////////////////////////////////////////
//              Functions	            //
//////////////////////////////////////////

UINT64
ForwardingGetNewOutputSourceTag();

DEBUGGER_OUTPUT_SOURCE_STATUS
ForwardingOpenOutputSource(PDEBUGGER_EVENT_FORWARDING SourceDescriptor);

DEBUGGER_OUTPUT_SOURCE_STATUS
ForwardingCloseOutputSource(PDEBUGGER_EVENT_FORWARDING SourceDescriptor);

BOOLEAN
ForwardingCheckAndPerformEventForwarding(UINT32 OperationCode,
                                         CHAR * Message,
                                         UINT32 MessageLength);

BOOLEAN
ForwardingWriteToFile(HANDLE FileHandle, CHAR * Message, UINT32 MessageLength);

BOOLEAN
ForwardingSendToNamedPipe(HANDLE NamedPipeHandle, CHAR * Message, UINT32 MessageLength);

BOOLEAN
ForwardingSendToTcpSocket(SOCKET TcpSocket, CHAR * Message, UINT32 MessageLength);

VOID *
ForwardingCreateOutputSource(DEBUGGER_EVENT_FORWARDING_TYPE SourceType,
                             const string &                 Description,
                             SOCKET *                       Socket,
                             HMODULE *                      Module);
