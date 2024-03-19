/**
 * @file Process.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of kernel debugger functions for processes
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
 * @brief handle process changes for cr3 registers
 * @param CoreId
 *
 * @return VOID
 */
VOID
ProcessTriggerCr3ProcessChange(UINT32 CoreId)
{
    PROCESSOR_DEBUGGING_STATE * DbgState = &g_DbgState[CoreId];

    //
    // Call kernel debugger handler for mov to cr3 in kernel debugger
    //
    if (DbgState->ThreadOrProcessTracingDetails.IsWatingForMovCr3VmExits)
    {
        ProcessHandleProcessChange(DbgState);
    }
}

/**
 * @brief handle process changes
 * @param DbgState The state of the debugger on the current core
 *
 * @return VOID
 */
BOOLEAN
ProcessHandleProcessChange(PROCESSOR_DEBUGGING_STATE * DbgState)
{
    //
    // Check if we reached to the target process or not
    //
    if ((g_ProcessSwitch.ProcessId != NULL_ZERO && g_ProcessSwitch.ProcessId == HANDLE_TO_UINT32(PsGetCurrentProcessId())) ||
        (g_ProcessSwitch.Process != (UINT64)NULL && g_ProcessSwitch.Process == PsGetCurrentProcess()))
    {
        KdHandleBreakpointAndDebugBreakpoints(DbgState, DEBUGGEE_PAUSING_REASON_DEBUGGEE_PROCESS_SWITCHED, NULL);

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
 * @brief make evnvironment ready to change the process
 *
 * @param DbgState The state of the debugger on the current core
 * @param ProcessId
 * @param EProcess
 * @param IsSwitchByClockIntrrupt
 *
 * @return BOOLEAN
 */
BOOLEAN
ProcessSwitch(PROCESSOR_DEBUGGING_STATE * DbgState,
              UINT32                      ProcessId,
              PEPROCESS                   EProcess,
              BOOLEAN                     IsSwitchByClockIntrrupt)
{
    //
    // Initialized with NULL
    //
    g_ProcessSwitch.Process   = NULL;
    g_ProcessSwitch.ProcessId = NULL_ZERO;

    //
    // Check to avoid invalid switch
    //
    if (ProcessId == NULL_ZERO && EProcess == (PEPROCESS)NULL)
    {
        return FALSE;
    }

    //
    // Set the target process id, eprocess to switch
    //
    if (EProcess != NULL)
    {
        if (CheckAccessValidityAndSafety((UINT64)EProcess, sizeof(BYTE)))
        {
            g_ProcessSwitch.Process = (PVOID)EProcess;
        }
        else
        {
            //
            // An invalid address is specified by user
            //
            return FALSE;
        }
    }
    else if (ProcessId != NULL_ZERO)
    {
        g_ProcessSwitch.ProcessId = ProcessId;
    }

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreBroadcastTaskAllCores(DbgState,
                                    DEBUGGER_HALTED_CORE_TASK_SET_PROCESS_INTERCEPTION,
                                    TRUE,
                                    TRUE,
                                    (PVOID)IsSwitchByClockIntrrupt);

    return TRUE;
}

/**
 * @brief Enable or disable the process change monitoring detection
 * on the running core based on intercepting clock interrupts
 * @details should be called on vmx root
 *
 * @param DbgState The state of the debugger on the current core
 * @param Enable
 * @return VOID
 */
VOID
ProcessDetectChangeByInterceptingClockInterrupts(PROCESSOR_DEBUGGING_STATE * DbgState,
                                                 BOOLEAN                     Enable)
{
    if (Enable)
    {
        //
        // Indicate that we're waiting for clock interrupt vm-exits
        //
        DbgState->ThreadOrProcessTracingDetails.InterceptClockInterruptsForProcessChange = TRUE;

        //
        // Set external-interrupt vm-exits
        //
        VmFuncSetExternalInterruptExiting(DbgState->CoreId, TRUE);
    }
    else
    {
        //
        // Indicate that we're not waiting for clock interrupt vm-exits
        //
        DbgState->ThreadOrProcessTracingDetails.InterceptClockInterruptsForProcessChange = FALSE;

        //
        // Unset external-interrupt vm-exits
        //
        VmFuncSetExternalInterruptExiting(DbgState->CoreId, FALSE);
    }
}

/**
 * @brief Enable or disable the process change monitoring detection
 * on the running core based on mov-to-cr3 vm-exits
 * @details should be called on vmx root
 *
 * @param DbgState The state of the debugger on the current core
 * @param Enable
 * @return VOID
 */
VOID
ProcessDetectChangeByMov2Cr3Vmexits(PROCESSOR_DEBUGGING_STATE * DbgState,
                                    BOOLEAN                     Enable)
{
    if (Enable)
    {
        //
        // Indicate that we're waiting for mov-to-cr3 vm-exits
        //
        DbgState->ThreadOrProcessTracingDetails.IsWatingForMovCr3VmExits = TRUE;

        //
        // Set mov to cr3 vm-exit, this flag is also use to remove the
        // mov 2 cr3 on next halt
        //
        VmFuncSetMovToCr3Vmexit(DbgState->CoreId, TRUE);
    }
    else
    {
        //
        // Indicate that we're not waiting for mov-to-cr3 vm-exits
        //
        DbgState->ThreadOrProcessTracingDetails.IsWatingForMovCr3VmExits = FALSE;

        //
        // Unset mov to cr3 vm-exit, this flag is also use to remove the
        // mov 2 cr3 on next halt
        //
        VmFuncSetMovToCr3Vmexit(DbgState->CoreId, FALSE);
    }
}

/**
 * @brief Enable or disable the process change monitoring detection
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
ProcessEnableOrDisableThreadChangeMonitor(PROCESSOR_DEBUGGING_STATE * DbgState,
                                          BOOLEAN                     Enable,
                                          BOOLEAN                     IsSwitchByClockIntrrupt)
{
    if (Enable)
    {
        //
        // Set indicator process interception on the target core
        //
        DbgState->ThreadOrProcessTracingDetails.InitialSetProcessChangeEvent = TRUE;
        DbgState->ThreadOrProcessTracingDetails.InitialSetByClockInterrupt   = IsSwitchByClockIntrrupt;
    }
    else
    {
        //
        // Avoid future sets/unsets
        //
        DbgState->ThreadOrProcessTracingDetails.InitialSetProcessChangeEvent = FALSE;
        DbgState->ThreadOrProcessTracingDetails.InitialSetByClockInterrupt   = FALSE;
    }

    //
    // Check whether we should intercept mov-to-cr3 vm-exits or intercept
    // the clock interrupts
    //
    if (!IsSwitchByClockIntrrupt)
    {
        ProcessDetectChangeByMov2Cr3Vmexits(DbgState, Enable);
    }
    else
    {
        ProcessDetectChangeByInterceptingClockInterrupts(DbgState, Enable);
    }
}

/**
 * @brief checks whether the given nt!_EPROCESS is valid or not
 * @param Eprocess target nt!_EPROCESS
 * @param ActiveProcessHead nt!PsActiveProcessHead
 * @param ActiveProcessLinksOffset nt!_EPROCESS.ActiveProcessLinks
 *
 * @return BOOLEAN
 */
BOOLEAN
ProcessCheckIfEprocessIsValid(UINT64 Eprocess, UINT64 ActiveProcessHead, ULONG ActiveProcessLinksOffset)
{
    UINT64     Process;
    LIST_ENTRY ActiveProcessLinks = {0};

    //
    // Dirty validation of parameters
    //
    if (ActiveProcessHead == NULL64_ZERO ||
        ActiveProcessLinksOffset == NULL_ZERO)
    {
        return FALSE;
    }

    //
    // Check if address is valid
    //
    if (CheckAccessValidityAndSafety(ActiveProcessHead, sizeof(BYTE)))
    {
        //
        // Show processes list, we read everything from the view of system
        // process
        //
        MemoryMapperReadMemorySafe(ActiveProcessHead, &ActiveProcessLinks, sizeof(ActiveProcessLinks));

        //
        // Find the top of EPROCESS from nt!_EPROCESS.ActiveProcessLinks
        //
        Process = (UINT64)ActiveProcessLinks.Flink - ActiveProcessLinksOffset;

        do
        {
            //
            // Read the next process
            //
            MemoryMapperReadMemorySafe(Process + ActiveProcessLinksOffset,
                                       &ActiveProcessLinks,
                                       sizeof(ActiveProcessLinks));

            //
            // Check if we find the process
            //
            if (Process == Eprocess)
            {
                return TRUE;
            }

            //
            // Find the next process from the list of this process
            //
            Process = (UINT64)ActiveProcessLinks.Flink - ActiveProcessLinksOffset;

        } while ((UINT64)ActiveProcessLinks.Flink != ActiveProcessHead);
    }
    else
    {
        //
        // An invalid address is specified
        //
        return FALSE;
    }

    return FALSE;
}

/**
 * @brief shows the processes list
 * @param PorcessListSymbolInfo
 * @param QueryAction
 * @param CountOfProcesses
 * @param ListSaveBuffer
 * @param ListSaveBuffSize
 *
 * @return BOOLEAN
 */
BOOLEAN
ProcessShowList(PDEBUGGEE_PROCESS_LIST_NEEDED_DETAILS              PorcessListSymbolInfo,
                DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTIONS QueryAction,
                UINT32 *                                           CountOfProcesses,
                PVOID                                              ListSaveBuffer,
                UINT64                                             ListSaveBuffSize)
{
    UINT64                               Process;
    UINT64                               UniquePid          = 0;
    LIST_ENTRY                           ActiveProcessLinks = {0};
    UCHAR                                ImageFileName[15]  = {0};
    CR3_TYPE                             ProcessCr3         = {0};
    UINT32                               EnumerationCount   = 0;
    UINT32                               MaximumBufferCount = 0;
    PDEBUGGEE_PROCESS_LIST_DETAILS_ENTRY SavingEntries      = ListSaveBuffer;

    //
    // validate parameters
    //
    if (QueryAction == DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_QUERY_COUNT &&
        CountOfProcesses == NULL)
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
        MaximumBufferCount = (UINT32)(ListSaveBuffSize / sizeof(DEBUGGEE_PROCESS_LIST_DETAILS_ENTRY));
    }

    //
    // Set the details derived from the symbols
    //
    UINT64 ActiveProcessHead        = PorcessListSymbolInfo->PsActiveProcessHead;      // nt!PsActiveProcessHead
    ULONG  ImageFileNameOffset      = PorcessListSymbolInfo->ImageFileNameOffset;      // nt!_EPROCESS.ImageFileName
    ULONG  UniquePidOffset          = PorcessListSymbolInfo->UniquePidOffset;          // nt!_EPROCESS.UniqueProcessId
    ULONG  ActiveProcessLinksOffset = PorcessListSymbolInfo->ActiveProcessLinksOffset; // nt!_EPROCESS.ActiveProcessLinks

    //
    // Dirty validation of parameters
    //
    if (ActiveProcessHead == NULL64_ZERO ||
        ImageFileNameOffset == NULL_ZERO ||
        UniquePidOffset == NULL_ZERO ||
        ActiveProcessLinksOffset == NULL_ZERO)
    {
        return FALSE;
    }

    //
    // Check if address is valid
    //
    if (CheckAccessValidityAndSafety(ActiveProcessHead, sizeof(BYTE)))
    {
        //
        // Show processes list, we read everything from the view of system
        // process
        //
        MemoryMapperReadMemorySafe(ActiveProcessHead, &ActiveProcessLinks, sizeof(ActiveProcessLinks));

        //
        // Find the top of EPROCESS from nt!_EPROCESS.ActiveProcessLinks
        //
        Process = (UINT64)ActiveProcessLinks.Flink - ActiveProcessLinksOffset;

        do
        {
            //
            // Read Process name, Process ID, CR3 of the target process
            //
            MemoryMapperReadMemorySafe(Process + ImageFileNameOffset,
                                       &ImageFileName,
                                       sizeof(ImageFileName));

            MemoryMapperReadMemorySafe(Process + UniquePidOffset,
                                       &UniquePid,
                                       sizeof(UniquePid));

            MemoryMapperReadMemorySafe(Process + ActiveProcessLinksOffset,
                                       &ActiveProcessLinks,
                                       sizeof(ActiveProcessLinks));

            //
            // Get the kernel CR3 for the target process
            //
            NT_KPROCESS * CurrentProcess = (NT_KPROCESS *)(Process);
            ProcessCr3.Flags             = CurrentProcess->DirectoryTableBase;

            //
            // Show the list of process
            //
            switch (QueryAction)
            {
            case DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_SHOW_INSTANTLY:

                Log("PROCESS\t%llx\n\tProcess Id: %04x\tDirBase (Kernel Cr3): %016llx\tImage: %s\n\n",
                    Process,
                    UniquePid,
                    ProcessCr3.Flags,
                    ImageFileName);

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
                SavingEntries[EnumerationCount - 1].Eprocess  = Process;
                SavingEntries[EnumerationCount - 1].ProcessId = (UINT32)UniquePid;
                SavingEntries[EnumerationCount - 1].Cr3       = ProcessCr3.Flags;
                RtlCopyMemory(&SavingEntries[EnumerationCount - 1].ImageFileName, ImageFileName, 15);

                break;

            default:

                LogError("Err, invalid action specified for process enumeration");

                break;
            }

            //
            // Find the next process from the list of this process
            //
            Process = (UINT64)ActiveProcessLinks.Flink - ActiveProcessLinksOffset;

        } while ((UINT64)ActiveProcessLinks.Flink != ActiveProcessHead);
    }
    else
    {
        //
        // An invalid address is specified by the debugger
        //
        return FALSE;
    }

ReturnEnd:

    //
    // In case of query count of processes, we'll set this parameter
    //
    if (QueryAction == DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_QUERY_COUNT)
    {
        *CountOfProcesses = EnumerationCount;
    }

    return TRUE;
}

/**
 * @brief change the current process
 * @detail ONLY TO BE USED IN KD STUFFS
 *
 * @param DbgState The state of the debugger on the current core
 * @param PidRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
ProcessInterpretProcess(PROCESSOR_DEBUGGING_STATE * DbgState, PDEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET PidRequest)
{
    switch (PidRequest->ActionType)
    {
    case DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_GET_PROCESS_DETAILS:

        //
        // Debugger wants to know current pid, nt!_EPROCESS and process name
        //
        PidRequest->ProcessId = HANDLE_TO_UINT32(PsGetCurrentProcessId());
        PidRequest->Process   = (UINT64)PsGetCurrentProcess();
        MemoryMapperReadMemorySafe((UINT64)CommonGetProcessNameFromProcessControlBlock(PsGetCurrentProcess()), &PidRequest->ProcessName, 16);

        //
        // Operation was successful
        //
        PidRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

        break;

    case DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PERFORM_SWITCH:

        //
        // Perform the process switch
        //
        if (!ProcessSwitch(DbgState, PidRequest->ProcessId, (PEPROCESS)PidRequest->Process, PidRequest->IsSwitchByClkIntr))
        {
            PidRequest->Result = DEBUGGER_ERROR_DETAILS_OR_SWITCH_PROCESS_INVALID_PARAMETER;
            break;
        }

        //
        // Operation was successful
        //
        PidRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

        break;

    case DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_GET_PROCESS_LIST:

        //
        // Show the process list
        //
        if (!ProcessShowList(&PidRequest->ProcessListSymDetails,
                             DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_SHOW_INSTANTLY,
                             NULL,
                             NULL,
                             (UINT64)NULL))
        {
            PidRequest->Result = DEBUGGER_ERROR_DETAILS_OR_SWITCH_PROCESS_INVALID_PARAMETER;
            break;
        }

        //
        // Operation was successful
        //
        PidRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

        break;

    default:

        //
        // Invalid type of action
        //
        PidRequest->Result = DEBUGGER_ERROR_DETAILS_OR_SWITCH_PROCESS_INVALID_PARAMETER;

        break;
    }

    //
    // Check if the above operation contains error
    //
    if (PidRequest->Result == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief Query process details (count)
 *
 * @param DebuggerUsermodeProcessOrThreadQueryRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
ProcessQueryCount(PDEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS DebuggerUsermodeProcessOrThreadQueryRequest)
{
    BOOLEAN Result = FALSE;

    //
    // Getting the count results
    //
    Result = ProcessShowList(&DebuggerUsermodeProcessOrThreadQueryRequest->ProcessListNeededDetails,
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
 * @brief Query process details (list)
 *
 * @param DebuggerUsermodeProcessOrThreadQueryRequest
 * @param AddressToSaveDetail
 * @param BufferSize
 *
 * @return BOOLEAN
 */
BOOLEAN
ProcessQueryList(PDEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS DebuggerUsermodeProcessOrThreadQueryRequest,
                 PVOID                                       AddressToSaveDetail,
                 UINT32                                      BufferSize)
{
    BOOLEAN Result = FALSE;

    //
    // Getting the count results
    //
    Result = ProcessShowList(&DebuggerUsermodeProcessOrThreadQueryRequest->ProcessListNeededDetails,
                             DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_QUERY_SAVE_DETAILS,
                             NULL,
                             AddressToSaveDetail,
                             BufferSize);

    return Result;
}

/**
 * @brief Query process details
 *
 * @param GetInformationProcessRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
ProcessQueryDetails(PDEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET GetInformationProcessRequest)
{
    GetInformationProcessRequest->ProcessId = HANDLE_TO_UINT32(PsGetCurrentProcessId());
    GetInformationProcessRequest->Process   = (UINT64)PsGetCurrentProcess();
    RtlCopyMemory(&GetInformationProcessRequest->ProcessName,
                  CommonGetProcessNameFromProcessControlBlock(PsGetCurrentProcess()),
                  15);

    GetInformationProcessRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

    return TRUE;
}
