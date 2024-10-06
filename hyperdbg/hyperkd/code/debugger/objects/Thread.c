/**
 * @file Thread.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of kernel debugger functions for threads
 * @details
 *
 * @version 0.1
 * @date 2021-11-23
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief handle thread changes
 * @param DbgState The state of the debugger on the current core
 *
 * @return BOOLEAN
 */
BOOLEAN
ThreadHandleThreadChange(PROCESSOR_DEBUGGING_STATE * DbgState)
{
    //
    // Check if we reached to the target thread or not
    //
    if ((g_ThreadSwitch.ThreadId != NULL_ZERO && g_ThreadSwitch.ThreadId == HANDLE_TO_UINT32(PsGetCurrentThreadId())) ||
        (g_ThreadSwitch.Thread != NULL64_ZERO && g_ThreadSwitch.Thread == PsGetCurrentThread()))
    {
        //
        // Halt the debuggee, we have found the target thread
        //
        KdHandleBreakpointAndDebugBreakpoints(DbgState, DEBUGGEE_PAUSING_REASON_DEBUGGEE_THREAD_SWITCHED, NULL, FALSE);

        //
        // Found
        //
        return TRUE;
    }

    //
    // Not found
    //
    return FALSE;
}

/**
 * @brief make evnvironment ready to change the thread
 *
 * @param DbgState The state of the debugger on the current core
 * @param ThreadId
 * @param EThread
 * @param CheckByClockInterrupt
 *
 * @return BOOLEAN
 */
BOOLEAN
ThreadSwitch(PROCESSOR_DEBUGGING_STATE * DbgState,
             UINT32                      ThreadId,
             PETHREAD                    EThread,
             BOOLEAN                     CheckByClockInterrupt)
{
    //
    // Initialized with NULL
    //
    g_ThreadSwitch.Thread   = NULL64_ZERO;
    g_ThreadSwitch.ThreadId = NULL_ZERO;

    //
    // Check to avoid invalid switch
    //
    if (ThreadId == NULL_ZERO && EThread == NULL64_ZERO)
    {
        return FALSE;
    }

    //
    // Set the target thread id, ethread to switch
    //
    if (EThread != NULL)
    {
        if (CheckAccessValidityAndSafety((UINT64)EThread, sizeof(BYTE)))
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
    else if (ThreadId != NULL_ZERO)
    {
        g_ThreadSwitch.ThreadId = ThreadId;
    }

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(DbgState,
                                    DEBUGGER_HALTED_CORE_TASK_SET_THREAD_INTERCEPTION,
                                    TRUE,
                                    TRUE,
                                    (PVOID)CheckByClockInterrupt);

    return TRUE;
}

/**
 * @brief shows the threads list
 * @param ThreadListSymbolInfo
 * @param QueryAction
 * @param CountOfThreads
 * @param ListSaveBuffer
 * @param ListSaveBuffSize
 *
 * @return BOOLEAN
 */
BOOLEAN
ThreadShowList(PDEBUGGEE_THREAD_LIST_NEEDED_DETAILS               ThreadListSymbolInfo,
               DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTIONS QueryAction,
               UINT32 *                                           CountOfThreads,
               PVOID                                              ListSaveBuffer,
               UINT64                                             ListSaveBuffSize)
{
    UINT64                              ThreadListHead;
    UINT32                              EnumerationCount   = 0;
    UINT64                              Thread             = (UINT64)NULL;
    LIST_ENTRY                          ThreadLinks        = {0};
    CLIENT_ID                           ThreadCid          = {0};
    UINT32                              MaximumBufferCount = 0;
    PDEBUGGEE_THREAD_LIST_DETAILS_ENTRY SavingEntries      = ListSaveBuffer;

    //
    // validate parameters
    //
    if (QueryAction == DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_QUERY_COUNT &&
        CountOfThreads == NULL)
    {
        return FALSE;
    }

    if (QueryAction == DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_QUERY_SAVE_DETAILS &&
        (ListSaveBuffer == NULL || ListSaveBuffSize == 0))
    {
        return FALSE;
    }

    //
    // compute size to avoid overflow
    //
    if (QueryAction == DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_QUERY_SAVE_DETAILS)
    {
        MaximumBufferCount = (UINT32)(ListSaveBuffSize / sizeof(DEBUGGEE_THREAD_LIST_DETAILS_ENTRY));
    }

    UINT32 ThreadListHeadOffset       = ThreadListSymbolInfo->ThreadListHeadOffset;     // nt!_EPROCESS.ThreadListHead
    UINT32 ThreadListEntryOffset      = ThreadListSymbolInfo->ThreadListEntryOffset;    // nt!_ETHREAD.ThreadListEntry
    UINT32 CidOffset                  = ThreadListSymbolInfo->CidOffset;                // nt!_ETHREAD.Cid
    UINT32 ActiveProcessLinksOffset   = ThreadListSymbolInfo->ActiveProcessLinksOffset; // nt!_EPROCESS.ActiveProcessLinks
    UINT64 PsActiveProcessHeadAddress = ThreadListSymbolInfo->PsActiveProcessHead;      // nt!PsActiveProcessHead

    //
    // Validate params
    //
    if (ThreadListHeadOffset == NULL_ZERO ||
        ThreadListEntryOffset == NULL_ZERO ||
        CidOffset == NULL_ZERO ||
        ActiveProcessLinksOffset == NULL_ZERO ||
        PsActiveProcessHeadAddress == NULL64_ZERO)
    {
        return FALSE;
    }

    //
    // Set the target process
    //
    if (ThreadListSymbolInfo->Process == NULL64_ZERO)
    {
        //
        // Means that it's for the current process
        //
        ThreadListSymbolInfo->Process = (UINT64)PsGetCurrentProcess();
        ThreadListHead                = (UINT64)PsGetCurrentProcess() + ThreadListHeadOffset;
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
    if (!CheckAccessValidityAndSafety(ThreadListHead, sizeof(BYTE)))
    {
        return FALSE;
    }

    //
    // Check if the nt!_EPROCESS is valid or not (available in the system or not)
    //
    if (!ProcessCheckIfEprocessIsValid(ThreadListSymbolInfo->Process,
                                       PsActiveProcessHeadAddress,
                                       ActiveProcessLinksOffset))
    {
        return FALSE;
    }

    if (QueryAction == DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_SHOW_INSTANTLY)
    {
        //
        // Show the message of show the process
        //
        Log("PROCESS\t%llx\tIMAGE\t%s\n",
            ThreadListSymbolInfo->Process,
            CommonGetProcessNameFromProcessControlBlock((PEPROCESS)ThreadListSymbolInfo->Process));
    }

    //
    // Show thread list, we read everything from the view of system process
    //
    MemoryMapperReadMemorySafe(ThreadListHead, &ThreadLinks, sizeof(ThreadLinks));

    //
    // Find the top of ETHREAD from nt!_ETHREAD.ThreadListEntry
    //
    Thread = (UINT64)ThreadLinks.Flink - ThreadListEntryOffset;

    do
    {
        //
        // Show thread list, we read everything from the view of system process
        //
        MemoryMapperReadMemorySafe(Thread + CidOffset,
                                   &ThreadCid,
                                   sizeof(ThreadCid));

        switch (QueryAction)
        {
        case DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_SHOW_INSTANTLY:

            //
            // Show the list of process
            //
            Log("\tTHREAD\t%llx (%llx.%llx)\n", Thread, ThreadCid.UniqueProcess, ThreadCid.UniqueThread);

            break;

        case DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_QUERY_COUNT:

            EnumerationCount++;

            break;

        case DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_QUERY_SAVE_DETAILS:

            EnumerationCount++;

            //
            // Check to avoid overflow
            //
            if (EnumerationCount == MaximumBufferCount - 1)
            {
                //
                // buffer is full
                //
                goto ReturnEnd;
            }

            //
            // Save the details
            //
            SavingEntries[EnumerationCount - 1].Eprocess  = ThreadListSymbolInfo->Process;
            SavingEntries[EnumerationCount - 1].ProcessId = HANDLE_TO_UINT32(ThreadCid.UniqueProcess);
            SavingEntries[EnumerationCount - 1].ThreadId  = HANDLE_TO_UINT32(ThreadCid.UniqueThread);
            SavingEntries[EnumerationCount - 1].Ethread   = Thread;

            RtlCopyMemory(&SavingEntries[EnumerationCount - 1].ImageFileName,
                          CommonGetProcessNameFromProcessControlBlock((PEPROCESS)ThreadListSymbolInfo->Process),
                          15);

            break;

        default:
            break;
        }

        MemoryMapperReadMemorySafe(Thread + ThreadListEntryOffset,
                                   &ThreadLinks,
                                   sizeof(ThreadLinks));

        //
        // Find the next process from the list of this process
        //
        Thread = (UINT64)ThreadLinks.Flink - ThreadListEntryOffset;

    } while ((UINT64)ThreadLinks.Flink != ThreadListHead);

ReturnEnd:
    //
    // In case of query count of Threads, we'll set this parameter
    //
    if (QueryAction == DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_QUERY_COUNT)
    {
        *CountOfThreads = EnumerationCount;
    }

    return TRUE;
}

/**
 * @brief change the current thread
 *
 * @param DbgState The state of the debugger on the current core
 * @param TidRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
ThreadInterpretThread(PROCESSOR_DEBUGGING_STATE *                DbgState,
                      PDEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET TidRequest)
{
    switch (TidRequest->ActionType)
    {
    case DEBUGGEE_DETAILS_AND_SWITCH_THREAD_GET_THREAD_DETAILS:

        //
        // Debugger wants to know current tid, nt!_ETHREAD and process name, etc.
        //
        TidRequest->ProcessId = HANDLE_TO_UINT32(PsGetCurrentProcessId());
        TidRequest->ThreadId  = HANDLE_TO_UINT32(PsGetCurrentThreadId());
        TidRequest->Process   = (UINT64)PsGetCurrentProcess();
        TidRequest->Thread    = (UINT64)PsGetCurrentThread();
        MemoryMapperReadMemorySafe((UINT64)CommonGetProcessNameFromProcessControlBlock(PsGetCurrentProcess()), &TidRequest->ProcessName, 16);

        //
        // Operation was successful
        //
        TidRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

        break;

    case DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PERFORM_SWITCH:

        //
        // Perform the thread switch
        //
        if (!ThreadSwitch(DbgState,
                          TidRequest->ThreadId,
                          (PETHREAD)TidRequest->Thread,
                          TidRequest->CheckByClockInterrupt))
        {
            TidRequest->Result = DEBUGGER_ERROR_DETAILS_OR_SWITCH_THREAD_INVALID_PARAMETER;
            break;
        }

        //
        // Operation was successful
        //
        TidRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

        break;

    case DEBUGGEE_DETAILS_AND_SWITCH_THREAD_GET_THREAD_LIST:

        //
        // Show the threads list
        //
        if (!ThreadShowList(&TidRequest->ThreadListSymDetails,
                            DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_SHOW_INSTANTLY,
                            NULL,
                            NULL,
                            (UINT64)NULL))
        {
            TidRequest->Result = DEBUGGER_ERROR_DETAILS_OR_SWITCH_THREAD_INVALID_PARAMETER;
            break;
        }

        //
        // Operation was successful
        //
        TidRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

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
    if (TidRequest->Result == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
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
 * on the running core based on putting a HW breakpoint on the gs:[188]
 * @details should be called on vmx root
 *
 * @param DbgState The state of the debugger on the current core
 * @param Enable
 * @return VOID
 */
VOID
ThreadDetectChangeByDebugRegisterOnGs(PROCESSOR_DEBUGGING_STATE * DbgState,
                                      BOOLEAN                     Enable)
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
        MsrGsBase = __readmsr(IA32_GS_BASE);

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
        DbgState->ThreadOrProcessTracingDetails.CurrentThreadLocationOnGs = MsrGsBase;

        //
        // Set interception state
        //
        DbgState->ThreadOrProcessTracingDetails.DebugRegisterInterceptionState = TRUE;

        //
        // Enable load debug controls and save debug controls because we don't
        // want dr7 and dr0 remove their configuration on vm-exits and also
        // we'll be able to change the dr7 of the guest on VMCS
        //
        VmFuncSetLoadDebugControls(TRUE);
        VmFuncSetSaveDebugControls(TRUE);

        //
        // Intercept #DBs by changing exception bitmap (one core)
        //
        VmFuncSetExceptionBitmap(DbgState->CoreId, EXCEPTION_VECTOR_DEBUG_BREAKPOINT);

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
        SetDebugRegisters(
            DEBUGGER_DEBUG_REGISTER_FOR_THREAD_MANAGEMENT,
            BREAK_ON_WRITE_ONLY,
            TRUE,
            DbgState->ThreadOrProcessTracingDetails.CurrentThreadLocationOnGs);

        //
        // Enables mov to debug registers exitings in primary cpu-based controls
        // it is because I realized that some other routines in Windows like
        // KiSaveProcessorControlState and KiRestoreProcessorControlState and
        // other functions directly change the debug registers, probably
        // because we should not modify debug registers directly, by the way, we
        // are hypervisor and we can easily ignore mov to debug register (0 in
        // this case), however we should somehow hide this process in the future
        //
        VmFuncSetMovDebugRegsExiting(DbgState->CoreId, TRUE);
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
        DbgState->ThreadOrProcessTracingDetails.DebugRegisterInterceptionState = FALSE;

        //
        // Disable mov to debug regs vm-exit
        //
        VmFuncSetMovDebugRegsExiting(DbgState->CoreId, FALSE);

        //
        // Disable load debug controls and save debug controls because
        // no longer needed
        //
        VmFuncSetLoadDebugControls(FALSE);
        VmFuncSetSaveDebugControls(FALSE);

        //
        // Disable intercepting #DBs
        //
        VmFuncUnsetExceptionBitmap(DbgState->CoreId, EXCEPTION_VECTOR_DEBUG_BREAKPOINT);

        //
        // No longer need to store such gs:188 value
        //
        DbgState->ThreadOrProcessTracingDetails.CurrentThreadLocationOnGs = (UINT64)NULL;
    }
}

/**
 * @brief Enable or disable the thread change monitoring detection
 * on the running core based on intercepting clock interrupts
 * @details should be called on vmx root
 *
 * @param DbgState The state of the debugger on the current core
 * @param Enable
 * @return VOID
 */
VOID
ThreadDetectChangeByInterceptingClockInterrupts(PROCESSOR_DEBUGGING_STATE * DbgState,
                                                BOOLEAN                     Enable)
{
    if (Enable)
    {
        //
        // We should get the clock interrupts
        //
        DbgState->ThreadOrProcessTracingDetails.InterceptClockInterruptsForThreadChange = TRUE;

        //
        // Intercept external interrupts (for monitoring clock interrupts)
        //
        VmFuncSetExternalInterruptExiting(DbgState->CoreId, TRUE);
    }
    else
    {
        //
        // We should ignore intercepting any further clock interrupts
        //
        DbgState->ThreadOrProcessTracingDetails.InterceptClockInterruptsForThreadChange = FALSE;

        //
        // Undo intercepting external interrupts
        //
        VmFuncSetExternalInterruptExiting(DbgState->CoreId, FALSE);
    }
}

/**
 * @brief Enable or disable the thread change monitoring detection
 * on the running core
 * @details should be called on vmx root
 *
 * @param DbgState The state of the debugger on the current core
 * @param Enable
 * @param IsSwitchByClockIntrrupt
 *
 * @return VOID
 */
VOID
ThreadEnableOrDisableThreadChangeMonitor(PROCESSOR_DEBUGGING_STATE * DbgState,
                                         BOOLEAN                     Enable,
                                         BOOLEAN                     IsSwitchByClockIntrrupt)
{
    if (Enable)
    {
        DbgState->ThreadOrProcessTracingDetails.InitialSetThreadChangeEvent = TRUE;
        DbgState->ThreadOrProcessTracingDetails.InitialSetByClockInterrupt  = IsSwitchByClockIntrrupt;
    }
    else
    {
        //
        // Avoid future sets/unsets
        //
        DbgState->ThreadOrProcessTracingDetails.InitialSetThreadChangeEvent = FALSE;
        DbgState->ThreadOrProcessTracingDetails.InitialSetByClockInterrupt  = FALSE;
    }

    //
    // Check if it's a HW breakpoint on gs:[188] or a clock interception
    //
    if (!IsSwitchByClockIntrrupt)
    {
        ThreadDetectChangeByDebugRegisterOnGs(DbgState, Enable);
    }
    else
    {
        ThreadDetectChangeByInterceptingClockInterrupts(DbgState, Enable);
    }
}

/**
 * @brief Query thread details (count)
 *
 * @param DebuggerUsermodeProcessOrThreadQueryRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
ThreadQueryCount(PDEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS DebuggerUsermodeProcessOrThreadQueryRequest)
{
    BOOLEAN Result = FALSE;

    //
    // Getting the count results
    //
    Result = ThreadShowList(&DebuggerUsermodeProcessOrThreadQueryRequest->ThreadListNeededDetails,
                            DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_QUERY_COUNT,
                            &DebuggerUsermodeProcessOrThreadQueryRequest->Count,
                            NULL,
                            (UINT64)NULL);

    if (Result && DebuggerUsermodeProcessOrThreadQueryRequest->Count != 0)
    {
        DebuggerUsermodeProcessOrThreadQueryRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
        return TRUE;
    }

    DebuggerUsermodeProcessOrThreadQueryRequest->Result = DEBUGGER_ERROR_UNABLE_TO_QUERY_COUNT_OF_PROCESSES_OR_THREADS;
    return FALSE;
}

/**
 * @brief Query thread details (list)
 *
 * @param DebuggerUsermodeProcessOrThreadQueryRequest
 * @param AddressToSaveDetail
 * @param BufferSize
 *
 * @return BOOLEAN
 */
BOOLEAN
ThreadQueryList(PDEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS DebuggerUsermodeProcessOrThreadQueryRequest,
                PVOID                                       AddressToSaveDetail,
                UINT32                                      BufferSize)
{
    BOOLEAN Result = FALSE;

    //
    // Getting the list of threads
    //
    Result = ThreadShowList(&DebuggerUsermodeProcessOrThreadQueryRequest->ThreadListNeededDetails,
                            DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_QUERY_SAVE_DETAILS,
                            NULL,
                            AddressToSaveDetail,
                            BufferSize);

    return Result;
}

/**
 * @brief Query thread details
 *
 * @param GetInformationThreadRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
ThreadQueryDetails(PDEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET GetInformationThreadRequest)
{
    GetInformationThreadRequest->ProcessId = HANDLE_TO_UINT32(PsGetCurrentProcessId());
    GetInformationThreadRequest->Process   = (UINT64)PsGetCurrentProcess();
    GetInformationThreadRequest->Thread    = (UINT64)PsGetCurrentThread();
    GetInformationThreadRequest->ThreadId  = HANDLE_TO_UINT32(PsGetCurrentThreadId());

    RtlCopyMemory(&GetInformationThreadRequest->ProcessName,
                  CommonGetProcessNameFromProcessControlBlock(PsGetCurrentProcess()),
                  15);

    GetInformationThreadRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

    return TRUE;
}
