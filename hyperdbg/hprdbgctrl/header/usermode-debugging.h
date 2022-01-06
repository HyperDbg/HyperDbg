/**
 * @file usermode-debugging.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief headers for user-mode debugging routines
 * @details
 * @version 0.1
 * @date 2021-12-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//            	    Structures                  //
//////////////////////////////////////////////////

/**
 * @brief structures related to debugging
 *
 */
typedef struct _THREAD_DEBUGGING_STATE
{
    UINT64     UniqueDebuggingId;
    BOOLEAN    IsAttachedToUsermodeProcess;
    UINT32     ProcessId;
    UINT32     ThreadId;
    BOOLEAN    IsPaused;
    GUEST_REGS Registers;
    UINT64     Context;  // $context
    UINT64     GuestRip; // if IsPaused is TRUE
    UINT64     BaseAddressOfMainModule;
    UINT64     EntrypointOfMainModule;
    BOOLEAN    Is32Bit;
    LIST_ENTRY ListOfAttachedThreads;

} THREAD_DEBUGGING_STATE, *PTHREAD_DEBUGGING_STATE;

//////////////////////////////////////////////////
//            	    Functions                  //
//////////////////////////////////////////////////

BOOL
UsermodeDebuggingListProcessThreads(DWORD OwnerPID);

BOOLEAN
UsermodeDebuggingCheckThreadByProcessId(DWORD Pid, DWORD Tid);

BOOLEAN
UsermodeDebuggingAttachToProcess(UINT32 TargetPid, UINT32 TargetTid, const WCHAR * TargetFileAddress);
