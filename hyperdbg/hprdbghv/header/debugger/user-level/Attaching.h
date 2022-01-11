/**
 * @file Attaching.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Header for attaching and detaching for debugging user-mode processes
 * @details 
 * @version 0.1
 * @date 2021-12-28
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//				   Constants					//
//////////////////////////////////////////////////

#define MAX_USER_ACTIONS_FOR_THREADS 10

//////////////////////////////////////////////////
//				     Enums			    		//
//////////////////////////////////////////////////

/**
 * @brief User-mode debugging actions
 * 
 */
typedef enum _USERMODE_DEBUGGING_ACTION_TYPE
{
    USERMODE_DEBUGGING_ACTION_TYPE_NONE = 0,
    USERMODE_DEBUGGING_ACTION_TYPE_PAUSE,
    USERMODE_DEBUGGING_ACTION_TYPE_CONTINUE,
    USERMODE_DEBUGGING_ACTION_TYPE_REGULAR_STEP_IN,
    USERMODE_DEBUGGING_ACTION_TYPE_REGULAR_STEP_OVER,

} USERMODE_DEBUGGING_ACTION_TYPE;

//////////////////////////////////////////////////
//				   Structures					//
//////////////////////////////////////////////////

/**
 * @brief Description of user-mode debugging actions
 * 
 */
typedef struct _USERMODE_DEBUGGING_ACTION
{
    USERMODE_DEBUGGING_ACTION_TYPE ActionType;
    UINT64                         OptionalParam1;
    UINT64                         OptionalParam2;
    UINT64                         OptionalParam3;
    UINT64                         OptionalParam4;
    PVOID                          OptionalBuffer;

} USERMODE_DEBUGGING_ACTION, *PUSERMODE_DEBUGGING_ACTION;

/**
 * @brief Description of each active thread in user-mode attaching 
 * mechanism
 * 
 */
typedef struct _USERMODE_DEBUGGING_THREADS_DETAILS
{
    UINT64                    Token;
    BOOLEAN                   Enabled;
    BOOLEAN                   IsPaused;
    PVOID                     PebAddressToMonitor;
    GUEST_REGS                Registers;
    UINT64                    Context;  // $context
    UINT64                    GuestRip; // if IsPaused is TRUE
    UINT64                    GuestRsp; // if IsPaused is TRUE
    LIST_ENTRY                AttachedThreadList;
    UINT64                    UsermodeReservedBuffer;
    UINT64                    EntrypointOfMainModule;
    UINT64                    BaseAddressOfMainModule;
    PEPROCESS                 Eprocess;
    UINT32                    ProcessId;
    UINT32                    ThreadId;
    BOOLEAN                   Is32Bit;
    BOOLEAN                   IsOnTheStartingPhase;
    USERMODE_DEBUGGING_ACTION UserAction[MAX_USER_ACTIONS_FOR_THREADS];

} USERMODE_DEBUGGING_THREADS_DETAILS, *PUSERMODE_DEBUGGING_THREADS_DETAILS;

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

BOOLEAN
AttachingInitialize();

VOID
AttachingTargetProcess(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS Request);

VOID
AttachingHandleEntrypointDebugBreak(UINT32 CurrentProcessorIndex, PGUEST_REGS GuestRegs);

VOID
AttachingRemoveAndFreeAllThreadDebuggingDetails();

PUSERMODE_DEBUGGING_THREADS_DETAILS
AttachingFindThreadDebuggingDetailsByToken(UINT64 Token);

PUSERMODE_DEBUGGING_THREADS_DETAILS
AttachingFindThreadDebuggingDetailsByProcessIdAndThreadId(UINT32 ProcessId, UINT32 ThreadId);
