/**
 * @file ud.h
 * @author Sina Karvandi (sina@hyperdbg.org)
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
    UINT64     Rip;
    BOOLEAN    Is32Bit;
    BYTE       InstructionBytesOnRip[MAXIMUM_INSTR_SIZE];

} ACTIVE_DEBUGGING_PROCESS, *PACTIVE_DEBUGGING_PROCESS;

//////////////////////////////////////////////////
//            	    Functions                  //
//////////////////////////////////////////////////

VOID
UdInitializeUserDebugger();

VOID
UdUninitializeUserDebugger();

VOID
UdRemoveActiveDebuggingProcess();

VOID
UdHandleUserDebuggerPausing(PDEBUGGEE_UD_PAUSED_PACKET PausePacket);

VOID
UdSendStepPacketToDebuggee(UINT64 ThreadDetailToken, UINT32 TargetThreadId, DEBUGGER_REMOTE_STEPPING_REQUEST StepType);

VOID
UdSetActiveDebuggingProcess(UINT64  DebuggingId,
                            UINT64  Rip,
                            UINT32  ProcessId,
                            UINT32  ThreadId,
                            BOOLEAN Is32Bit,
                            BOOLEAN IsPaused,
                            BYTE    InstructionBytesOnRip[]);

BOOLEAN
UdSetActiveDebuggingThreadByPidOrTid(UINT32 TargetPidOrTid, BOOLEAN IsTid);

BOOLEAN
UdSetActiveDebuggingThreadByPidOrTid(UINT32 TargetPidOrTid, BOOLEAN IsTid);

BOOLEAN
UdShowListActiveDebuggingProcessesAndThreads();

BOOLEAN
UdListProcessThreads(DWORD OwnerPID);

BOOLEAN
UdCheckThreadByProcessId(DWORD Pid, DWORD Tid);

BOOLEAN
UdAttachToProcess(UINT32        TargetPid,
                  const WCHAR * TargetFileAddress,
                  const WCHAR * CommandLine,
                  BOOLEAN       RunCallbackAtTheFirstInstruction);

BOOLEAN
UdKillProcess(UINT32 TargetPid);

BOOLEAN
UdDetachProcess(UINT32 TargetPid, UINT64 ProcessDetailToken);

BOOLEAN
UdContinueProcess(UINT64 ProcessDebuggingToken);

BOOLEAN
UdPauseProcess(UINT64 ProcessDebuggingToken);

BOOLEAN
UdSendCommand(UINT64                          ProcessDetailToken,
              UINT32                          ThreadId,
              DEBUGGER_UD_COMMAND_ACTION_TYPE ActionType,
              PVOID                           OptionalBuffer,
              UINT32                          OptionalBufferSize,
              BOOLEAN                         ApplyToAllPausedThreads,
              BOOLEAN                         WaitForEventCompletion,
              UINT64                          OptionalParam1,
              UINT64                          OptionalParam2,
              UINT64                          OptionalParam3,
              UINT64                          OptionalParam4);

BOOLEAN
UdSendScriptBufferToProcess(UINT64  ProcessDetailToken,
                            UINT32  TargetThreadId,
                            UINT64  BufferAddress,
                            UINT32  BufferLength,
                            UINT32  Pointer,
                            BOOLEAN IsFormat);

BOOLEAN
UdSendReadRegisterToUserDebugger(UINT64                              ProcessDetailToken,
                                 UINT32                              TargetThreadId,
                                 PDEBUGGEE_REGISTER_READ_DESCRIPTION RegDes,
                                 UINT32                              RegBuffSize);
