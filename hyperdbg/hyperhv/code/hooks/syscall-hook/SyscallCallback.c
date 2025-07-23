/**
 * @file SyscallCallback.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of the functions related to the callback for Syscall
 * @details
 *
 * @version 0.14
 * @date 2025-06-07
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Initialize the syscall callback
 *
 * @return BOOLEAN
 */
BOOLEAN
SyscallCallbackInitialize()
{
    MSR Msr = {0};

    //
    // Check whether the syscall callback was already initialized or not
    //
    if (!g_SyscallCallbackStatus)
    {
        //
        // Insert EPT memory page hook for Windows system call handler, KiSystemCall64()
        //
        Msr.Flags = __readmsr(IA32_LSTAR);

        //
        // We set the hook at the address of the system call handler + 3
        // because we don't want to hook the first 3 bytes of the system call handler
        // which is SWAPGS instruction
        //
        g_SystemCallHookAddress = (PVOID)(Msr.Flags + 3);

        //
        // Apply the hook from vmx non-root mode
        //
        if (!ConfigureEptHook(g_SystemCallHookAddress, (UINT32)(ULONG_PTR)PsGetCurrentProcessId()))
        {
            // LogInfo("Error while inserting EPT page hook for Windows system call handler at address 0x%p+3", Msr.Flags);

            return FALSE;
        }

        //
        // Allocate buffer for the syscall callback trap flag state
        //
        g_SyscallCallbackTrapFlagState = (SYSCALL_CALLBACK_TRAP_FLAG_STATE *)PlatformMemAllocateZeroedNonPagedPool(sizeof(SYSCALL_CALLBACK_TRAP_FLAG_STATE));

        //
        // Intercept trap flags #DBs and #BPs for the syscall callback
        //
        BroadcastEnableDbAndBpExitingAllCores();

        //
        // Enable the syscall callback
        //
        g_SyscallCallbackStatus = TRUE;

        //
        // Successfully enabled the syscall callback
        //
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief Uninitialize the syscall callback
 *
 * @return BOOLEAN
 */
BOOLEAN
SyscallCallbackUninitialize()
{
    if (g_SyscallCallbackStatus)
    {
        //
        // Disable the syscall callback
        //
        g_SyscallCallbackStatus = FALSE;

        //
        // Unset the trap flags #DBs and #BPs for the syscall callback
        //
        BroadcastDisableDbAndBpExitingAllCores();

        //
        // Free the buffer for the syscall callback trap flag state
        //
        PlatformMemFreePool(g_SyscallCallbackTrapFlagState);

        MSR Msr   = {0};
        Msr.Flags = __readmsr(IA32_LSTAR);

        if (!ConfigureEptHookUnHookSingleAddress((UINT64)(Msr.Flags + 3), (UINT64)NULL, (UINT32)(ULONG_PTR)PsGetCurrentProcessId()))
        {
            LogInfo("Error while removing the EPT hook from windows syscall handler at address 0x%p+3", Msr.Flags);

            return FALSE;
        }

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief This function makes sure to unset the RFLAGS.TF on next trigger of #DB
 * on the target process/thread
 * @param ProcessId
 * @param ThreadId
 * @param Context
 * @param Params
 *
 * @return BOOLEAN
 */
BOOLEAN
SyscallCallbackStoreProcessInformation(UINT32                            ProcessId,
                                       UINT32                            ThreadId,
                                       UINT64                            Context,
                                       SYSCALL_CALLBACK_CONTEXT_PARAMS * Params)
{
    UINT32                                      Index;
    BOOLEAN                                     Result;
    BOOLEAN                                     SuccessfullyStored;
    SYSCALL_CALLBACK_PROCESS_THREAD_INFORMATION ProcThrdInfo = {0};

    //
    // Form the process id and thread id into a 64-bit value
    //
    ProcThrdInfo.Fields.ProcessId = ProcessId;
    ProcThrdInfo.Fields.ThreadId  = ThreadId;

    //
    // Make sure, nobody is in the middle of modifying the list
    //
    SpinlockLock(&SyscallCallbackModeTrapListLock);

    //
    // *** Search the list of processes/threads for the current process's trap flag state ***
    //
    Result = BinarySearchPerformSearchItem((UINT64 *)&g_SyscallCallbackTrapFlagState->ThreadInformation[0],
                                           g_SyscallCallbackTrapFlagState->NumberOfItems,
                                           &Index,
                                           ProcThrdInfo.asUInt);

    if (Result)
    {
        //
        // It means that we already find this entry in the stored list
        // so, just imply that the addition was successful (no need for extra addition)
        //
        SuccessfullyStored = TRUE;
        goto Return;
    }
    else
    {
        //
        // Insert the thread into the list as the item is not already present
        //
        SuccessfullyStored = InsertionSortInsertItem((UINT64 *)&g_SyscallCallbackTrapFlagState->ThreadInformation[0],
                                                     &g_SyscallCallbackTrapFlagState->NumberOfItems,
                                                     MAXIMUM_NUMBER_OF_THREAD_INFORMATION_FOR_SYSCALL_CALLBACK_TRAPS,
                                                     &Index,
                                                     ProcThrdInfo.asUInt);

        if (SuccessfullyStored)
        {
            //
            // Successfully inserted the thread/process into the list
            // Now let's store the context of the caller along with parameters
            //
            g_SyscallCallbackTrapFlagState->Context[Index] = Context;
            memcpy(&g_SyscallCallbackTrapFlagState->Params[Index], Params, sizeof(SYSCALL_CALLBACK_CONTEXT_PARAMS));
        }

        goto Return;
    }

Return:
    //
    // Unlock the list modification lock
    //
    SpinlockUnlock(&SyscallCallbackModeTrapListLock);

    return SuccessfullyStored;
}

/**
 * @brief Set the trap flag in the guest after a syscall
 *
 * @param Regs The virtual processor's state of registers
 * @param ProcessId The process id of the thread
 * @param ThreadId The thread id of the thread
 * @param Context The context of the caller
 * @param Params The (optional) parameters of the caller
 *
 * @return BOOLEAN
 */
BOOLEAN
SyscallCallbackSetTrapFlagAfterSyscall(GUEST_REGS *                      Regs,
                                       UINT32                            ProcessId,
                                       UINT32                            ThreadId,
                                       UINT64                            Context,
                                       SYSCALL_CALLBACK_CONTEXT_PARAMS * Params)
{
    //
    // Do not add anything to the list if the syscall callback is not enabled (or disabled by the user)
    //
    if (!g_SyscallCallbackStatus)
    {
        //
        // syscall callback is not enabled
        //
        return FALSE;
    }

    //
    // Insert the thread/process into the list of processes/threads
    //
    if (!SyscallCallbackStoreProcessInformation(ProcessId, ThreadId, Context, Params))
    {
        //
        // Failed to store the process/thread information
        //
        return FALSE;
    }

    //
    // *** Successfully stored the process/thread information ***
    //

    //
    // Set the trap flag to TRUE because we want to intercept the thread again
    // once it returns to the user-mode (SYSRET) instruction
    //
    // Here the RFLAGS is in the R11 register (See Intel manual about the SYSCALL register)
    //
    Regs->r11 |= X86_FLAGS_TF;

    //
    // Create log message for the syscall
    //
    // LogInfo("Syscall callback set trap flag for process: %x, thread: %x\n", ProcessId, ThreadId);

    return TRUE;
}

/**
 * @brief Handle the trap flags as the result of interception of the return of the
 * system-call
 *
 * @param VCpu The virtual processor's state
 * @param ProcessId The process id of the thread
 * @param ThreadId The thread id of the thread
 *
 * @return BOOLEAN
 */
BOOLEAN
SyscallCallbackCheckAndHandleAfterSyscallTrapFlags(VIRTUAL_MACHINE_STATE * VCpu,
                                                   UINT32                  ProcessId,
                                                   UINT32                  ThreadId)
{
    RFLAGS                                      Rflags = {0};
    UINT32                                      Index;
    UINT64                                      Context = NULL64_ZERO;
    SYSCALL_CALLBACK_CONTEXT_PARAMS             Params;
    SYSCALL_CALLBACK_PROCESS_THREAD_INFORMATION ProcThrdInfo = {0};
    BOOLEAN                                     Result;
    BOOLEAN                                     ResultToReturn;

    //
    // Read the trap flag
    //
    Rflags.AsUInt = HvGetRflags();

    if (!Rflags.TrapFlag)
    {
        //
        // The trap flag is not set, so we don't need to do anything
        //
        return FALSE;
    }

    //
    // Form the process id and thread id into a 64-bit value
    //
    ProcThrdInfo.Fields.ProcessId = ProcessId;
    ProcThrdInfo.Fields.ThreadId  = ThreadId;

    //
    // Make sure, nobody is in the middle of modifying the list
    //
    SpinlockLock(&SyscallCallbackModeTrapListLock);

    //
    // *** Search the list of processes/threads for the current process's trap flag state ***
    //
    Result = BinarySearchPerformSearchItem((UINT64 *)&g_SyscallCallbackTrapFlagState->ThreadInformation[0],
                                           g_SyscallCallbackTrapFlagState->NumberOfItems,
                                           &Index,
                                           ProcThrdInfo.asUInt);

    //
    // Check whether this thread is expected to have trap flag
    // by the syscall callback or not
    //
    if (Result)
    {
        //
        // Read the context of the caller
        //
        Context = g_SyscallCallbackTrapFlagState->Context[Index];

        //
        // Read the (optional) parameters of the caller
        //
        memcpy(&Params, &g_SyscallCallbackTrapFlagState->Params[Index], sizeof(SYSCALL_CALLBACK_CONTEXT_PARAMS));

        //
        // Clear the trap flag from the RFLAGS register
        //
        HvSetRflagTrapFlag(FALSE);

        //
        // Remove the thread/process from the list of processes/threads
        //
        InsertionSortDeleteItem((UINT64 *)&g_SyscallCallbackTrapFlagState->ThreadInformation[0],
                                &g_SyscallCallbackTrapFlagState->NumberOfItems,
                                Index);

        //
        // Handled by the syscall callback
        //
        ResultToReturn = TRUE;

        goto ReturnResult;
    }
    else
    {
        //
        // Not related to the syscall callback
        //
        ResultToReturn = FALSE;

        goto ReturnResult;
    }

ReturnResult:

    //
    // Unlock the list modification lock
    //
    SpinlockUnlock(&SyscallCallbackModeTrapListLock);

    //
    // Call the callback function to handle the trap flag if its needed
    // Note that we call it here so we already unlocked the list lock
    // to optimize the performance (avoid holding the lock for a long time)
    //
    if (ResultToReturn)
    {
        TransparentCallbackHandleAfterSyscall(VCpu->Regs, ProcessId, ThreadId, Context, &Params);
    }

    return ResultToReturn;
}

/**
 * @brief Handle the system call hook callback
 *
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
SyscallCallbackHandleSystemCallHook(VIRTUAL_MACHINE_STATE * VCpu)
{
    TransparentHandleSystemCallHook(VCpu->Regs);
}
