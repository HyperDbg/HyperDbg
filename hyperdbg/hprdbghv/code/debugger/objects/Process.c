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
 * @brief handle process changes
 * @param ProcessorIndex Index of processor
 * @param GuestState Guest's gp registers
 * 
 * 
 * @return VOID 
 */
BOOLEAN
ProcessHandleProcessChange(UINT32 ProcessorIndex, PGUEST_REGS GuestState)
{
    //
    // Check if we reached to the target process or not
    //
    if ((g_ProcessSwitch.ProcessId != NULL && g_ProcessSwitch.ProcessId == PsGetCurrentProcessId()) ||
        (g_ProcessSwitch.Process != NULL && g_ProcessSwitch.Process == PsGetCurrentProcess()))
    {
        KdHandleBreakpointAndDebugBreakpoints(ProcessorIndex, GuestState, DEBUGGEE_PAUSING_REASON_DEBUGGEE_PROCESS_SWITCHED, NULL);

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
 * @param ProcessId
 * @param EProcess
 * @param IsSwitchByClockIntrrupt
 * 
 * @return BOOLEAN 
 */
BOOLEAN
ProcessSwitch(UINT32 ProcessId, PEPROCESS EProcess, BOOLEAN IsSwitchByClockIntrrupt)
{
    ULONG CoreCount = 0;

    //
    // Initialized with NULL
    //
    g_ProcessSwitch.Process   = NULL;
    g_ProcessSwitch.ProcessId = NULL;

    //
    // Check to avoid invalid switch
    //
    if (ProcessId == NULL && EProcess == NULL)
    {
        return FALSE;
    }

    //
    // Set the target process id, eprocess to switch
    //
    if (EProcess != NULL)
    {
        if (CheckMemoryAccessSafety(EProcess, sizeof(BYTE)))
        {
            g_ProcessSwitch.Process = EProcess;
        }
        else
        {
            //
            // An invalid address is specified by user
            //
            return FALSE;
        }
    }
    else if (ProcessId != NULL)
    {
        g_ProcessSwitch.ProcessId = ProcessId;
    }

    CoreCount = KeQueryActiveProcessorCount(0);

    //
    // Check the switching method
    //
    if (!IsSwitchByClockIntrrupt)
    {
        //
        // Set mov-cr3 vmexit for all the cores
        //
        for (size_t i = 0; i < CoreCount; i++)
        {
            g_GuestState[i].DebuggingState.ThreadOrProcessTracingDetails.InitialSetProcessChangeEvent = TRUE;
            g_GuestState[i].DebuggingState.ThreadOrProcessTracingDetails.InitialSetByClockInterrupt   = FALSE;
        }
    }
    else
    {
        //
        // This is based on clk interrupt
        //
        for (size_t i = 0; i < CoreCount; i++)
        {
            g_GuestState[i].DebuggingState.ThreadOrProcessTracingDetails.InitialSetProcessChangeEvent = TRUE;
            g_GuestState[i].DebuggingState.ThreadOrProcessTracingDetails.InitialSetByClockInterrupt   = TRUE;
        }
    }

    return TRUE;
}

/**
 * @brief Enable or disable the process change monitoring detection 
 * on the running core based on intercepting clock interrupts
 * @details should be called on vmx root
 * 
 * @param CurrentProcessorIndex 
 * @param Enable 
 * @return VOID 
 */
VOID
ProcessDetectChangeByInterceptingClockInterrupts(UINT32  CurrentProcessorIndex,
                                                 BOOLEAN Enable)
{
    if (Enable)
    {
        //
        // Indicate that we're waiting for clock interrupt vm-exits
        //
        g_GuestState[CurrentProcessorIndex].DebuggingState.ThreadOrProcessTracingDetails.InterceptClockInterruptsForProcessChange = TRUE;

        //
        // Set external-interrupt vm-exits
        //
        HvSetExternalInterruptExiting(TRUE);
    }
    else
    {
        //
        // Indicate that we're not waiting for clock interrupt vm-exits
        //
        g_GuestState[CurrentProcessorIndex].DebuggingState.ThreadOrProcessTracingDetails.InterceptClockInterruptsForProcessChange = FALSE;

        //
        // Unset external-interrupt vm-exits
        //
        HvSetExternalInterruptExiting(FALSE);
    }
}

/**
 * @brief Enable or disable the process change monitoring detection 
 * on the running core based on mov-to-cr3 vm-exits
 * @details should be called on vmx root
 * 
 * @param CurrentProcessorIndex 
 * @param Enable 
 * @return VOID 
 */
VOID
ProcessDetectChangeByMov2Cr3Vmexits(UINT32  CurrentProcessorIndex,
                                    BOOLEAN Enable)
{
    if (Enable)
    {
        //
        // Indicate that we're waiting for mov-to-cr3 vm-exits
        //
        g_GuestState[CurrentProcessorIndex].DebuggingState.ThreadOrProcessTracingDetails.IsWatingForMovCr3VmExits = TRUE;

        //
        // Set mov to cr3 vm-exit, this flag is also use to remove the
        // mov 2 cr3 on next halt
        //
        HvSetMovToCr3Vmexit(TRUE);
    }
    else
    {
        //
        // Indicate that we're not waiting for mov-to-cr3 vm-exits
        //
        g_GuestState[CurrentProcessorIndex].DebuggingState.ThreadOrProcessTracingDetails.IsWatingForMovCr3VmExits = FALSE;

        //
        // Unset mov to cr3 vm-exit, this flag is also use to remove the
        // mov 2 cr3 on next halt
        //
        HvSetMovToCr3Vmexit(FALSE);
    }
}

/**
 * @brief Enable or disable the process change monitoring detection 
 * on the running core
 * @details should be called on vmx root
 * 
 * @param CurrentProcessorIndex 
 * @param Enable 
 * @param CheckByClockInterrupts 
 * @return VOID 
 */
VOID
ProcessEnableOrDisableThreadChangeMonitor(UINT32  CurrentProcessorIndex,
                                          BOOLEAN Enable,
                                          BOOLEAN CheckByClockInterrupts)
{
    //
    // Check whether we should intercept mov-to-cr3 vm-exits or intercept
    // the clock interrupts
    //
    if (!CheckByClockInterrupts)
    {
        ProcessDetectChangeByMov2Cr3Vmexits(CurrentProcessorIndex, Enable);
    }
    else
    {
        ProcessDetectChangeByInterceptingClockInterrupts(CurrentProcessorIndex, Enable);
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
    LIST_ENTRY ActiveProcessLinks;

    //
    // Dirty validation of parameters
    //
    if (ActiveProcessHead == NULL ||
        ActiveProcessLinksOffset == NULL)
    {
        return FALSE;
    }

    //
    // Check if address is valid
    //
    if (CheckMemoryAccessSafety(ActiveProcessHead, sizeof(BYTE)))
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
    UINT64                               UniquePid;
    LIST_ENTRY                           ActiveProcessLinks;
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
        MaximumBufferCount = ListSaveBuffSize / sizeof(DEBUGGEE_PROCESS_LIST_DETAILS_ENTRY);
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
    if (ActiveProcessHead == NULL ||
        ImageFileNameOffset == NULL ||
        UniquePidOffset == NULL ||
        ActiveProcessLinksOffset == NULL)
    {
        return FALSE;
    }

    //
    // Check if address is valid
    //
    if (CheckMemoryAccessSafety(ActiveProcessHead, sizeof(BYTE)))
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
                SavingEntries[EnumerationCount - 1].Eprocess = Process;
                SavingEntries[EnumerationCount - 1].Pid      = UniquePid;
                SavingEntries[EnumerationCount - 1].Cr3      = ProcessCr3.Flags;
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
 * @param PidRequest
 * 
 * @return BOOLEAN 
 */
BOOLEAN
ProcessInterpretProcess(PDEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET PidRequest)
{
    switch (PidRequest->ActionType)
    {
    case DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_GET_PROCESS_DETAILS:

        //
        // Debugger wants to know current pid, nt!_EPROCESS and process name
        //
        PidRequest->ProcessId = PsGetCurrentProcessId();
        PidRequest->Process   = PsGetCurrentProcess();
        MemoryMapperReadMemorySafe(GetProcessNameFromEprocess(PsGetCurrentProcess()), &PidRequest->ProcessName, 16);

        //
        // Operation was successful
        //
        PidRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;

        break;

    case DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PERFORM_SWITCH:

        //
        // Perform the process switch
        //
        if (!ProcessSwitch(PidRequest->ProcessId, PidRequest->Process, PidRequest->IsSwitchByClkIntr))
        {
            PidRequest->Result = DEBUGGER_ERROR_DETAILS_OR_SWITCH_PROCESS_INVALID_PARAMETER;
            break;
        }

        //
        // Operation was successful
        //
        PidRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;

        break;

    case DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_GET_PROCESS_LIST:

        //
        // Show the process list
        //
        if (!ProcessShowList(&PidRequest->ProcessListSymDetails,
                             DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_ACTION_SHOW_INSTANTLY,
                             NULL,
                             NULL,
                             NULL))
        {
            PidRequest->Result = DEBUGGER_ERROR_DETAILS_OR_SWITCH_PROCESS_INVALID_PARAMETER;
            break;
        }

        //
        // Operation was successful
        //
        PidRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;

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
    if (PidRequest->Result == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
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
                             NULL);

    if (Result && DebuggerUsermodeProcessOrThreadQueryRequest->Count != 0)
    {
        DebuggerUsermodeProcessOrThreadQueryRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
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
    GetInformationProcessRequest->ProcessId = PsGetCurrentProcessId();
    GetInformationProcessRequest->Process   = PsGetCurrentProcess();
    RtlCopyMemory(&GetInformationProcessRequest->ProcessName,
                  GetProcessNameFromEprocess(PsGetCurrentProcess()),
                  15);

    GetInformationProcessRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;

    return TRUE;
}
