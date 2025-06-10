/**
 * @file SyscallHook.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for syscall hook callbacks
 * @details
 *
 * @version 0.14
 * @date 2025-06-07
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//////////////////////////////////////////////////
//				      Locks 	    			//
//////////////////////////////////////////////////

/**
 * @brief The lock for modifying list of process/thread for syscall callback trap flags
 *
 */
volatile LONG SyscallCallbackModeTrapListLock;

//////////////////////////////////////////////////
//				   Definitions					//
//////////////////////////////////////////////////

/**
 * @brief maximum number of thread/process ids to be allocated for keeping track of
 * of the trap flag
 *
 */
#define MAXIMUM_NUMBER_OF_THREAD_INFORMATION_FOR_SYSCALL_CALLBACK_TRAPS 500

//////////////////////////////////////////////////
//				    Structures  				//
//////////////////////////////////////////////////

/**
 * @brief The thread/process information
 *
 */
typedef struct _SYSCALL_CALLBACK_PROCESS_THREAD_INFORMATION
{
    union
    {
        UINT64 asUInt;

        struct
        {
            UINT32 ProcessId;
            UINT32 ThreadId;
        } Fields;
    };

} SYSCALL_CALLBACK_PROCESS_THREAD_INFORMATION, *PSYSCALL_CALLBACK_PROCESS_THREAD_INFORMATION;

/**
 * @brief The threads that we expect to get the trap flag
 *
 * @details Used for keeping track of the threads that we expect to get the trap flag
 *
 */
typedef struct _SYSCALL_CALLBACK_TRAP_FLAG_STATE
{
    UINT32                                      NumberOfItems;
    SYSCALL_CALLBACK_PROCESS_THREAD_INFORMATION ThreadInformation[MAXIMUM_NUMBER_OF_THREAD_INFORMATION_FOR_SYSCALL_CALLBACK_TRAPS];
    UINT64                                      Context[MAXIMUM_NUMBER_OF_THREAD_INFORMATION_FOR_SYSCALL_CALLBACK_TRAPS];
    SYSCALL_CALLBACK_CONTEXT_PARAMS             Params[MAXIMUM_NUMBER_OF_THREAD_INFORMATION_FOR_SYSCALL_CALLBACK_TRAPS];

} SYSCALL_CALLBACK_TRAP_FLAG_STATE, *PSYSCALL_CALLBACK_TRAP_FLAG_STATE;

//////////////////////////////////////////////////
//				    Functions   				//
//////////////////////////////////////////////////

BOOLEAN
SyscallCallbackInitialize();

BOOLEAN
SyscallCallbackUninitialize();

BOOLEAN
SyscallCallbackSetTrapFlagAfterSyscall(GUEST_REGS *                      Regs,
                                       UINT32                            ProcessId,
                                       UINT32                            ThreadId,
                                       UINT64                            Context,
                                       SYSCALL_CALLBACK_CONTEXT_PARAMS * Params);

BOOLEAN
SyscallCallbackCheckAndHandleAfterSyscallTrapFlags(VIRTUAL_MACHINE_STATE * VCpu,
                                                   UINT32                  ProcessId,
                                                   UINT32                  ThreadId);

VOID
SyscallCallbackHandleSystemCallHook(VIRTUAL_MACHINE_STATE * VCpu);
