/**
 * @file Thread.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Implementation of kernel debugger functions for threads
 * @details
 * 
 * @version 0.1
 * @date 2021-11-23
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "..\hprdbghv\pch.h"

/**
 * @brief handle thread changes
 * @param CurrentCore
 * @param GuestState
 * 
 * @return BOOLEAN 
 */
BOOLEAN
ThreadHandleThreadChange(UINT32 CurrentCore, PGUEST_REGS GuestState)
{
    //
    // Check if we reached to the target thread or not
    //
    if ((g_ThreadSwitch.ThreadId != NULL && g_ThreadSwitch.ThreadId == PsGetCurrentThreadId()) ||
        (g_ThreadSwitch.Thread != NULL && g_ThreadSwitch.Thread == PsGetCurrentThread()))
    {
        //
        // Halt the debuggee, we have found the target thread
        //
        KdHandleBreakpointAndDebugBreakpoints(CurrentCore, GuestState, DEBUGGEE_PAUSING_REASON_DEBUGGEE_THREAD_SWITCHED, NULL);

        return TRUE;
    }

    return FALSE;
}

/**
 * @brief make evnvironment ready to change the thread
 * @param ThreadId
 * @param EThread
 * 
 * @return BOOLEAN 
 */
BOOLEAN
ThreadSwitch(UINT32 ThreadId, PETHREAD EThread)
{
    ULONG CoreCount = 0;

    //
    // Initialized with NULL
    //
    g_ThreadSwitch.Thread   = NULL;
    g_ThreadSwitch.ThreadId = NULL;

    //
    // Check to avoid invalid switch
    //
    if (ThreadId == NULL && EThread == NULL)
    {
        return FALSE;
    }

    //
    // Set the target thread id, ethread to switch
    //
    if (EThread != NULL)
    {
        if (CheckMemoryAccessSafety(EThread, sizeof(BYTE)))
        {
            g_ThreadSwitch.Thread = EThread;
        }
        else
        {
            //
            // An invalid address is specified by user
            //
            return FALSE;
        }
    }
    else if (ThreadId != NULL)
    {
        g_ThreadSwitch.ThreadId = ThreadId;
    }

    //
    // Set the change thread alert for all the cores
    //
    CoreCount = KeQueryActiveProcessorCount(0);

    for (size_t i = 0; i < CoreCount; i++)
    {
        g_GuestState[i].DebuggingState.SetThreadChangeEvent = TRUE;
    }

    return TRUE;
}

/**
 * @brief shows the threads list
 * @param ThreadListSymbolInfo
 * 
 * @return BOOLEAN 
 */
BOOLEAN
ThreadShowList(PDEBUGGEE_THREAD_LIST_NEEDED_DETAILS ThreadListSymbolInfo)
{
    ULONG64    Thread = NULL;
    UINT64     ThreadListHead;
    LIST_ENTRY ThreadLinks = {0};

    UINT64 ThreadListHeadOffset  = ThreadListSymbolInfo->ThreadListHeadOffset;  // nt!_EPROCESS.ThreadListHead
    UINT64 ThreadListEntryOffset = ThreadListSymbolInfo->ThreadListEntryOffset; // nt!_ETHREAD.ThreadListEntry
    DbgBreakPoint();
    //
    // Validate params
    //
    if (ThreadListHeadOffset == 0 || ThreadListEntryOffset == 0)
    {
        return FALSE;
    }

    //
    // Set the target process
    //
    if (ThreadListSymbolInfo->Process == NULL)
    {
        //
        // Means that it's for the current process
        //
        ThreadListHead = (UINT64)PsGetCurrentProcess() + ThreadListHeadOffset;
    }
    else
    {
        //
        // Means that the user specified a special process
        //
        ThreadListHead = (UINT64)ThreadListSymbolInfo->Process + ThreadListHeadOffset;
    }

    //
    // Check if the process's thread list head is valid or not
    //
    if (!CheckMemoryAccessSafety(ThreadListHead, sizeof(BYTE)))
    {
        return FALSE;
    }

    //
    // Show thread list, we read everything from the view of system process
    //
    MemoryMapperReadMemorySafe(ThreadListHead, &ThreadLinks, sizeof(ThreadLinks));

    //
    // Find the top of ETHREAD from nt!_ETHREAD.ThreadListEntry
    //
    Thread = (ULONG64)ThreadLinks.Flink - ThreadListEntryOffset;

    do
    {
        //
        // Show the list of process
        //
        Log("THREAD\t%llx\n", Thread);

        MemoryMapperReadMemorySafe(Thread + ThreadListEntryOffset,
                                   &ThreadLinks,
                                   sizeof(ThreadLinks));

        //
        // Find the next process from the list of this process
        //
        Thread = (ULONG64)ThreadLinks.Flink - ThreadListEntryOffset;

    } while ((ULONG64)ThreadLinks.Flink != ThreadListHead);

    return TRUE;
}

/**
 * @brief change the current thread
 * @param TidRequest
 * 
 * @return BOOLEAN 
 */
BOOLEAN
ThreadInterpretThread(PDEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET TidRequest)
{
    switch (TidRequest->ActionType)
    {
    case DEBUGGEE_DETAILS_AND_SWITCH_THREAD_GET_THREAD_DETAILS:

        //
        // Debugger wants to know current tid, nt!_ETHREAD and process name, etc.
        //
        TidRequest->ProcessId = PsGetCurrentProcessId();
        TidRequest->ThreadId  = PsGetCurrentThreadId();
        TidRequest->Process   = PsGetCurrentProcess();
        TidRequest->Thread    = PsGetCurrentThread();
        MemoryMapperReadMemorySafe(GetProcessNameFromEprocess(PsGetCurrentProcess()), &TidRequest->ProcessName, 16);

        //
        // Operation was successful
        //
        TidRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;

        break;

    case DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PERFORM_SWITCH:

        //
        // Perform the thread switch
        //
        if (!ThreadSwitch(TidRequest->ThreadId, TidRequest->Thread))
        {
            TidRequest->Result = DEBUGGER_ERROR_DETAILS_OR_SWITCH_THREAD_INVALID_PARAMETER;
            break;
        }

        //
        // Operation was successful
        //
        TidRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;

        break;

    case DEBUGGEE_DETAILS_AND_SWITCH_THREAD_GET_THREAD_LIST:

        //
        // Show the threads list
        //
        if (!ThreadShowList(&TidRequest->ThreadListSymDetails))
        {
            TidRequest->Result = DEBUGGER_ERROR_DETAILS_OR_SWITCH_THREAD_INVALID_PARAMETER;
            break;
        }

        //
        // Operation was successful
        //
        TidRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;

        break;

    default:

        //
        // Invalid type of action
        //
        TidRequest->Result = DEBUGGER_ERROR_DETAILS_OR_SWITCH_THREAD_INVALID_PARAMETER;

        break;
    }

    //
    // Check if the above operation contains error
    //
    if (TidRequest->Result == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief Enable or disable the thread change monitoring detection 
 * on the running core
 * @details should be called on vmx root
 * 
 * @param Enable 
 * @return VOID 
 */
VOID
ThreadEnableOrDisableThreadChangeMonitorOnSingleCore(UINT32 CurrentProcessorIndex, BOOLEAN Enable)
{
    UINT64 MsrGsBase;

    if (Enable)
    {
        //
        // Enable Thread Change Detection
        // *** Read the address of GS:188 to g_CurrentThreadLocation ***
        //

        //
        // We are in kernel, so we should read MSR GS_BASE
        // IA32_GS_BASE             0xC0000101
        // IA32_KERNEL_GS_BASE      0xC0000102
        // IA32_KERNEL_GS_BASE is currently user's gs as
        // we are in the kernel-mode, if we need to intercept
        // from user-mode then IA32_KERNEL_GS_BASE should be used
        // but it's not for our case
        //
        MsrGsBase = __readmsr(MSR_GS_BASE);

        //
        // Now, we have the gs base on MSR
        // while in Windows gs:188 has the address
        // of where the store current _ETHREAD, so
        // we have to 188 to the gs base
        //
        MsrGsBase += 0x188;

        //
        // Set the global value for current thread of this processor
        //
        g_GuestState[CurrentProcessorIndex].DebuggingState.ThreadTracingDetails.CurrentThreadLocationOnGs = MsrGsBase;

        //
        // Set interception state
        //
        g_GuestState[CurrentProcessorIndex].DebuggingState.ThreadTracingDetails.DebugRegisterInterceptionState = TRUE;

        //
        // Enable load debug controls and save debug controls because we don't
        // want dr7 and dr0 remove their configuration on vm-exits and also
        // we'll be able to change the dr7 of the guest on VMCS
        //
        HvSetLoadDebugControls(TRUE);
        HvSetSaveDebugControls(TRUE);

        //
        // Intercept #DBs by changing exception bitmap (one core)
        //
        HvSetExceptionBitmap(EXCEPTION_VECTOR_DEBUG_BREAKPOINT);

        //
        // Note, this function is running as a DPC routines, means that
        // we're currently on DISPATCH_LEVEL so after modifying debug
        // registers and after disabling mov to dr (using vmcall),
        // nothing is able to change debug registers so it's safe
        //

        //
        // Set debug register to fire an exception in the case of
        // read/write on the gs:188 as we intercept it on
        // hypervisor side by exception bitmap on #DBs
        // However, this call is somehow useless because I also set it
        // on Mov 2 Debug regs handler (vm-exit), but we set from here
        // to make sure that the vm-exit handler set this break on access
        //
        DebugRegistersSet(
            DEBUGGER_DEBUG_REGISTER_FOR_THREAD_MANAGEMENT,
            BREAK_ON_WRITE_ONLY,
            TRUE,
            g_GuestState[CurrentProcessorIndex].DebuggingState.ThreadTracingDetails.CurrentThreadLocationOnGs);

        //
        // Enables mov to debug registers exitings in primary cpu-based controls
        // it is because I realized that some other routines in Windows like
        // KiSaveProcessorControlState and KiRestoreProcessorControlState and
        // other functions directly change the debug registers, probably
        // because we should not modify debug registers directly, by the way, we
        // are hypervisor and we can easily ignore mov to debug register (0 in
        // this case), however we should somehow hide this process in the future
        //
        HvSetMovDebugRegsExiting(TRUE);
    }
    else
    {
        //
        // Disable Thread Change Detection
        // *** Remove side changes ***
        //

        //
        // We should not ignore debug registers change anymore
        //
        g_GuestState[CurrentProcessorIndex].DebuggingState.ThreadTracingDetails.DebugRegisterInterceptionState = FALSE;

        //
        // Disable mov to debug regs vm-exit
        //
        HvSetMovDebugRegsExiting(FALSE);

        //
        // Disable load debug controls and save debug controls because
        // no longer needed
        //
        HvSetLoadDebugControls(FALSE);
        HvSetSaveDebugControls(FALSE);

        //
        // Disable intercepting #DBs
        //
        HvUnsetExceptionBitmap(EXCEPTION_VECTOR_DEBUG_BREAKPOINT);

        //
        // No longer need to store such gs:188 value
        //
        g_GuestState[CurrentProcessorIndex].DebuggingState.ThreadTracingDetails.CurrentThreadLocationOnGs = NULL;
    }
}
