/**
 * @file Definition.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header files for global definitions
 * @details This file contains definitions that are use in both user mode and
 * kernel mode Means that if you change the following files, structures or
 * enums, then these settings apply to both usermode and kernel mode
 * @version 0.1
 * @date 2020-04-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//
// IA32-doc has structures for the entire intel SDM.
//

#define USE_LIB_IA32
#if defined(USE_LIB_IA32)
#    pragma warning(push, 0)
//#    pragma warning(disable : 4201) // suppress nameless struct/union warning
#    include <ia32-doc/out/ia32.h>
#    pragma warning(pop)
typedef RFLAGS * PRFLAGS;
#endif // USE_LIB_IA32

//////////////////////////////////////////////////
//                Config File                  //
//////////////////////////////////////////////////

/**
 * @brief Config file name for HyperDbg
 *
 */
#define CONFIG_FILE_NAME L"config.ini"

//////////////////////////////////////////////////
//                   Installer                  //
//////////////////////////////////////////////////

/**
 * @brief name of HyperDbg's VMM driver
 *
 */
#define VMM_DRIVER_NAME "hprdbghv"

//////////////////////////////////////////////////
//				   Test Cases                   //
//////////////////////////////////////////////////

/**
 * @brief Query constant to show detail of halting of core
 */
#define TEST_QUERY_HALTING_CORE_STATUS 1

/**
 * @brief Test cases file name
 */
#define TEST_CASE_FILE_NAME "test-cases.txt"

/**
 * @brief Test cases file name
 */
#define SCRIPT_ENGINE_TEST_CASES_DIRECTORY "script-test-cases"

/**
 * @brief Maximum test cases to communicate between debugger and debuggee process
 */
#define TEST_CASE_MAXIMUM_NUMBER_OF_KERNEL_TEST_CASES 200

/**
 * @brief Maximum buffer to communicate between debugger and debuggee process
 */
#define TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE sizeof(DEBUGGEE_KERNEL_AND_USER_TEST_INFORMATION) * TEST_CASE_MAXIMUM_NUMBER_OF_KERNEL_TEST_CASES

//////////////////////////////////////////////////
//               Event Details                  //
//////////////////////////////////////////////////

/**
 * @brief Each command is like the following struct, it also used for
 * tracing works in user mode and sending it to the kernl mode
 * @details THIS IS NOT WHAT HYPERDBG SAVES FOR EVENTS IN KERNEL MODE
 */
typedef struct _DEBUGGER_GENERAL_EVENT_DETAIL
{
    LIST_ENTRY
    CommandsEventList; // Linked-list of commands list (used for tracing purpose
                       // in user mode)

    time_t CreationTime; // Date of creating this event

    UINT32 CoreId; // determines the core index to apply this event to, if it's
                   // 0xffffffff means that we have to apply it to all cores

    UINT32 ProcessId; // determines the process id to apply this to
                      // only that 0xffffffff means that we have to
                      // apply it to all processes

    BOOLEAN IsEnabled;

    BOOLEAN HasCustomOutput; // Shows whether this event has a custom output
                             // source or not

    UINT64
    OutputSourceTags
        [DebuggerOutputSourceMaximumRemoteSourceForSingleEvent]; // tags of
                                                                 // multiple
                                                                 // sources which
                                                                 // can be used to
                                                                 // send the event
                                                                 // results of
                                                                 // scripts to
                                                                 // remote sources

    UINT32 CountOfActions;

    UINT64                   Tag; // is same as operation code
    DEBUGGER_EVENT_TYPE_ENUM EventType;

    UINT64 OptionalParam1;
    UINT64 OptionalParam2;
    UINT64 OptionalParam3;
    UINT64 OptionalParam4;

    PVOID CommandStringBuffer;

    UINT32 ConditionBufferSize;

} DEBUGGER_GENERAL_EVENT_DETAIL, *PDEBUGGER_GENERAL_EVENT_DETAIL;

/**
 * @brief Each event can have mulitple actions
 * @details THIS STRUCTURE IS ONLY USED IN USER MODE
 * WE USE SEPARATE STRUCTURE FOR ACTIONS IN
 * KERNEL MODE
 */
typedef struct _DEBUGGER_GENERAL_ACTION
{
    UINT64                          EventTag;
    DEBUGGER_EVENT_ACTION_TYPE_ENUM ActionType;
    BOOLEAN                         ImmediateMessagePassing;
    UINT32                          PreAllocatedBuffer;

    UINT32 CustomCodeBufferSize;
    UINT32 ScriptBufferSize;
    UINT32 ScriptBufferPointer;

} DEBUGGER_GENERAL_ACTION, *PDEBUGGER_GENERAL_ACTION;

/**
 * @brief Status of register buffers
 *
 */
typedef struct _DEBUGGER_EVENT_AND_ACTION_REG_BUFFER
{
    BOOLEAN IsSuccessful;
    UINT32  Error; // If IsSuccessful was, FALSE

} DEBUGGER_EVENT_AND_ACTION_REG_BUFFER, *PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER;

#define SIZEOF_REGISTER_EVENT sizeof(REGISTER_NOTIFY_BUFFER)

/**
 * @brief Type of transferring buffer between user-to-kernel
 *
 */
typedef enum _NOTIFY_TYPE
{
    IRP_BASED,
    EVENT_BASED
} NOTIFY_TYPE;

/**
 * @brief Used to register event for transferring buffer between user-to-kernel
 *
 */
typedef struct _REGISTER_NOTIFY_BUFFER
{
    NOTIFY_TYPE Type;
    HANDLE      hEvent;

} REGISTER_NOTIFY_BUFFER, *PREGISTER_NOTIFY_BUFFER;
