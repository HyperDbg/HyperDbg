/**
 * @file ErrorCodes.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg's SDK Error codes
 * @details This file contains definitions of error codes used in HyperDbg
 * @version 0.2
 * @date 2022-06-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//		    	  Success Codes                 //
//////////////////////////////////////////////////

/**
 * @brief General value to indicate that the operation or
 * request was successful
 *
 */
#define DEBUGGER_OPERATION_WAS_SUCCESSFULL 0xFFFFFFFF

//////////////////////////////////////////////////
//		    	   Error Codes                  //
//////////////////////////////////////////////////

/**
 * @brief error, the tag not exist
 *
 */
#define DEBUGGER_ERROR_TAG_NOT_EXISTS 0xc0000000

/**
 * @brief error, invalid type of action
 *
 */
#define DEBUGGER_ERROR_INVALID_ACTION_TYPE 0xc0000001

/**
 * @brief error, the action buffer size is invalid
 *
 */
#define DEBUGGER_ERROR_ACTION_BUFFER_SIZE_IS_ZERO 0xc0000002

/**
 * @brief error, the event type is unknown
 *
 */
#define DEBUGGER_ERROR_EVENT_TYPE_IS_INVALID 0xc0000003

/**
 * @brief error, enable to create event
 *
 */
#define DEBUGGER_ERROR_UNABLE_TO_CREATE_EVENT 0xc0000004

/**
 * @brief error, invalid address specified for debugger
 *
 */
#define DEBUGGER_ERROR_INVALID_ADDRESS 0xc0000005

/**
 * @brief error, the core id is invalid
 *
 */
#define DEBUGGER_ERROR_INVALID_CORE_ID 0xc0000006

/**
 * @brief error, the index is greater than 32 in !exception command
 *
 */
#define DEBUGGER_ERROR_EXCEPTION_INDEX_EXCEED_FIRST_32_ENTRIES 0xc0000007

/**
 * @brief error, the index for !interrupt command is not between 32 to 256
 *
 */
#define DEBUGGER_ERROR_INTERRUPT_INDEX_IS_NOT_VALID 0xc0000008

/**
 * @brief error, unable to hide the debugger and enter to transparent-mode
 *
 */
#define DEBUGGER_ERROR_UNABLE_TO_HIDE_OR_UNHIDE_DEBUGGER 0xc0000009

/**
 * @brief error, the debugger is already in transparent-mode
 *
 */
#define DEBUGGER_ERROR_DEBUGGER_ALREADY_UHIDE 0xc000000a

/**
 * @brief error, invalid parameters in !e* e* commands
 *
 */
#define DEBUGGER_ERROR_EDIT_MEMORY_STATUS_INVALID_PARAMETER 0xc000000b

/**
 * @brief error, an invalid address is specified based on current cr3
 * in !e* or e* commands
 *
 */
#define DEBUGGER_ERROR_EDIT_MEMORY_STATUS_INVALID_ADDRESS_BASED_ON_CURRENT_PROCESS \
    0xc000000c

/**
 * @brief error, an invalid address is specified based on anotehr process's cr3
 * in !e* or e* commands
 *
 */
#define DEBUGGER_ERROR_EDIT_MEMORY_STATUS_INVALID_ADDRESS_BASED_ON_OTHER_PROCESS \
    0xc000000d

/**
 * @brief error, invalid tag for 'events' command (tag id is unknown for kernel)
 *
 */
#define DEBUGGER_ERROR_MODIFY_EVENTS_INVALID_TAG 0xc000000e

/**
 * @brief error, type of action (enable/disable/clear) is wrong
 *
 */
#define DEBUGGER_ERROR_MODIFY_EVENTS_INVALID_TYPE_OF_ACTION 0xc000000f

/**
 * @brief error, invalid parameters steppings actions
 *
 */
#define DEBUGGER_ERROR_STEPPING_INVALID_PARAMETER 0xc0000010

/**
 * @brief error, thread is invalid (not found) or disabled in
 * stepping (step-in & step-out) requests
 *
 */
#define DEBUGGER_ERROR_STEPPINGS_EITHER_THREAD_NOT_FOUND_OR_DISABLED 0xc0000011

/**
 * @brief error, baud rate is invalid
 *
 */
#define DEBUGGER_ERROR_PREPARING_DEBUGGEE_INVALID_BAUDRATE 0xc0000012

/**
 * @brief error, serial port address is invalid
 *
 */
#define DEBUGGER_ERROR_PREPARING_DEBUGGEE_INVALID_SERIAL_PORT 0xc0000013

/**
 * @brief error, invalid core selected in changing core in remote debuggee
 *
 */
#define DEBUGGER_ERROR_PREPARING_DEBUGGEE_INVALID_CORE_IN_REMOTE_DEBUGGE \
    0xc0000014

/**
 * @brief error, invalid process selected in changing process in remote debuggee
 *
 */
#define DEBUGGER_ERROR_PREPARING_DEBUGGEE_UNABLE_TO_SWITCH_TO_NEW_PROCESS \
    0xc0000015

/**
 * @brief error, unable to run script in remote debuggee
 *
 */
#define DEBUGGER_ERROR_PREPARING_DEBUGGEE_TO_RUN_SCRIPT 0xc0000016

/**
 * @brief error, invalid register number
 *
 */
#define DEBUGGER_ERROR_INVALID_REGISTER_NUMBER 0xc0000017

/**
 * @brief error, maximum pools were used without continueing debuggee
 *
 */
#define DEBUGGER_ERROR_MAXIMUM_BREAKPOINT_WITHOUT_CONTINUE 0xc0000018

/**
 * @brief error, breakpoint already exists on the target address
 *
 */
#define DEBUGGER_ERROR_BREAKPOINT_ALREADY_EXISTS_ON_THE_ADDRESS 0xc0000019

/**
 * @brief error, breakpoint id not found
 *
 */
#define DEBUGGER_ERROR_BREAKPOINT_ID_NOT_FOUND 0xc000001a

/**
 * @brief error, breakpoint already disabled
 *
 */
#define DEBUGGER_ERROR_BREAKPOINT_ALREADY_DISABLED 0xc000001b

/**
 * @brief error, breakpoint already enabled
 *
 */
#define DEBUGGER_ERROR_BREAKPOINT_ALREADY_ENABLED 0xc000001c

/**
 * @brief error, memory type is invalid
 *
 */
#define DEBUGGER_ERROR_MEMORY_TYPE_INVALID 0xc000001d

/**
 * @brief error, the process id is invalid
 *
 */
#define DEBUGGER_ERROR_INVALID_PROCESS_ID 0xc000001e

/**
 * @brief error, for event specific reasons the event is not
 * applied
 *
 */
#define DEBUGGER_ERROR_EVENT_IS_NOT_APPLIED 0xc000001f

/**
 * @brief error, for process switch or process details, invalid parameter
 *
 */
#define DEBUGGER_ERROR_DETAILS_OR_SWITCH_PROCESS_INVALID_PARAMETER 0xc0000020

/**
 * @brief error, for thread switch or thread details, invalid parameter
 *
 */
#define DEBUGGER_ERROR_DETAILS_OR_SWITCH_THREAD_INVALID_PARAMETER 0xc0000021

/**
 * @brief error, maximum breakpoint for a single page is hit
 *
 */
#define DEBUGGER_ERROR_MAXIMUM_BREAKPOINT_FOR_A_SINGLE_PAGE_IS_HIT 0xc0000022

/**
 * @brief error, there is no pre-allocated buffer
 *
 */
#define DEBUGGER_ERROR_PRE_ALLOCATED_BUFFER_IS_EMPTY 0xc0000023

/**
 * @brief error, in the EPT handler, it could not split the 2MB pages to
 * 512 entries of 4 KB pages
 *
 */
#define DEBUGGER_ERROR_EPT_COULD_NOT_SPLIT_THE_LARGE_PAGE_TO_4KB_PAGES 0xc0000024

/**
 * @brief error, failed to get PML1 entry of the target address
 *
 */
#define DEBUGGER_ERROR_EPT_FAILED_TO_GET_PML1_ENTRY_OF_TARGET_ADDRESS 0xc0000025

/**
 * @brief error, multiple EPT Hooks or Monitors are applied on a single page
 *
 */
#define DEBUGGER_ERROR_EPT_MULTIPLE_HOOKS_IN_A_SINGLE_PAGE 0xc0000026

/**
 * @brief error, could not build the EPT Hook
 *
 */
#define DEBUGGER_ERROR_COULD_NOT_BUILD_THE_EPT_HOOK 0xc0000027

/**
 * @brief error, could not find the type of allocation
 *
 */
#define DEBUGGER_ERROR_COULD_NOT_FIND_ALLOCATION_TYPE 0xc0000028

/**
 * @brief error, could not find the index of test query
 *
 */
#define DEBUGGER_ERROR_INVALID_TEST_QUERY_INDEX 0xc0000029

/**
 * @brief error, failed to attach to the target user-mode process
 *
 */
#define DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS 0xc000002a

/**
 * @brief error, failed to remove hooks as entrypoint is not reached yet
 * @details The caller of this functionality should keep sending the previous
 * IOCTL until the hook is remove successfully
 *
 */
#define DEBUGGER_ERROR_UNABLE_TO_REMOVE_HOOKS_ENTRYPOINT_NOT_REACHED 0xc000002b

/**
 * @brief error, could not remove the previous hook
 *
 */
#define DEBUGGER_ERROR_UNABLE_TO_REMOVE_HOOKS 0xc000002c

/**
 * @brief error, the needed routines for debugging is not initialized
 *
 */
#define DEBUGGER_ERROR_FUNCTIONS_FOR_INITIALIZING_PEB_ADDRESSES_ARE_NOT_INITIALIZED 0xc000002d

/**
 * @brief error, unable to get 32-bit or 64-bit of the target process
 *
 */
#define DEBUGGER_ERROR_UNABLE_TO_DETECT_32_BIT_OR_64_BIT_PROCESS 0xc000002e

/**
 * @brief error, unable to kill the target process
 *
 */
#define DEBUGGER_ERROR_UNABLE_TO_KILL_THE_PROCESS 0xc000002f

/**
 * @brief error, invalid thread debugging token
 *
 */
#define DEBUGGER_ERROR_INVALID_THREAD_DEBUGGING_TOKEN 0xc0000030

/**
 * @brief error, unable to pause the process's threads
 *
 */
#define DEBUGGER_ERROR_UNABLE_TO_PAUSE_THE_PROCESS_THREADS 0xc0000031

/**
 * @brief error, user debugger already attached to this process
 *
 */
#define DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_AN_ALREADY_ATTACHED_PROCESS 0xc0000032

/**
 * @brief error, the user debugger is not attached to the target process
 *
 */
#define DEBUGGER_ERROR_THE_USER_DEBUGGER_NOT_ATTACHED_TO_THE_PROCESS 0xc0000033

/**
 * @brief error, cannot detach from the process as there are paused threads
 *
 */
#define DEBUGGER_ERROR_UNABLE_TO_DETACH_AS_THERE_ARE_PAUSED_THREADS 0xc0000034

/**
 * @brief error, cannot switch to new thread as the process id or thread id is not found
 *
 */
#define DEBUGGER_ERROR_UNABLE_TO_SWITCH_PROCESS_ID_OR_THREAD_ID_IS_INVALID 0xc0000035

/**
 * @brief error, cannot switch to new thread the process doesn't contain an active thread
 *
 */
#define DEBUGGER_ERROR_UNABLE_TO_SWITCH_THERE_IS_NO_THREAD_ON_THE_PROCESS 0xc0000036

/**
 * @brief error, unable to get modules
 *
 */
#define DEBUGGER_ERROR_UNABLE_TO_GET_MODULES_OF_THE_PROCESS 0xc0000037

/**
 * @brief error, unable to get the callstack
 *
 */
#define DEBUGGER_ERROR_UNABLE_TO_GET_CALLSTACK 0xc0000038

/**
 * @brief error, unable to query count of processes or threads
 *
 */
#define DEBUGGER_ERROR_UNABLE_TO_QUERY_COUNT_OF_PROCESSES_OR_THREADS 0xc0000039

//
// WHEN YOU ADD ANYTHING TO THIS LIST OF ERRORS, THEN
// MAKE SURE TO ADD AN ERROR MESSAGE TO ShowErrorMessage(UINT32 Error)
// FUNCTION
//
