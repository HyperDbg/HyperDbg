/**
 * @file ud.h
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
 * @brief structures related to current thread debugging
 * state
 *
 */
typedef struct _ACTIVE_DEBUGGING_PROCESS
{
    BOOLEAN    IsActive;
    UINT64     ProcessDebuggingToken;
    UINT32     ProcessId;
    UINT32     ThreadId;
    BOOLEAN    IsPaused;
    GUEST_REGS Registers; // thread registers
    UINT64     Context;   // $context
    BOOLEAN    Is32Bit;

} ACTIVE_DEBUGGING_PROCESS, *PACTIVE_DEBUGGING_PROCESS;

//////////////////////////////////////////////////
//            	    Functions                  //
//////////////////////////////////////////////////

BOOL
UdListProcessThreads(DWORD OwnerPID);

BOOLEAN
UdCheckThreadByProcessId(DWORD Pid, DWORD Tid);

BOOLEAN
UdAttachToProcess(UINT32        TargetPid,
                  const WCHAR * TargetFileAddress,
                  WCHAR *       CommandLine);

BOOLEAN
UdKillProcess(UINT32 TargetPid);

BOOLEAN
UdPauseProcess(UINT64 ProcessDebuggingToken);

VOID
UdHandleUserDebuggerPausing(PDEBUGGEE_UD_PAUSED_PACKET PausePacket);

VOID
UdContinueDebuggee(UINT64 ProcessDetailToken);

VOID
UdSendStepPacketToDebuggee(UINT64 ThreadDetailToken, UINT32 TargetThreadId, DEBUGGER_REMOTE_STEPPING_REQUEST StepType);
