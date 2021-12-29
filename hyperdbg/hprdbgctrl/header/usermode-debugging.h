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
typedef struct _DEBUGGING_STATE
{
    BOOLEAN IsAttachedToUsermodeProcess;
    UINT64  ConnectedProcessId;
    UINT64  ConnectedThreadId;
} DEBUGGING_STATE, *PDEBUGGING_STATE;

//////////////////////////////////////////////////
//            	    Functions                  //
//////////////////////////////////////////////////

BOOL
UsermodeDebuggingListProcessThreads(DWORD OwnerPID);

BOOLEAN
UsermodeDebuggingCheckThreadByProcessId(DWORD Pid, DWORD Tid);

BOOLEAN
UsermodeDebuggingAttachToProcess(UINT32 TargetPid, UINT32 TargetTid, const WCHAR * TargetFileAddress);
