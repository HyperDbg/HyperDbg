/**
 * @file Process.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Implementation of kernel debugger functions for processes
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
 * @brief move to cr3 execute
 * @param ProcessorIndex Index of processor
 * @param GuestState Guest's gp registers
 * @param NewCr3
 * 
 * 
 * @return VOID 
 */
VOID
ProcessHandleMovToCr3(UINT32 ProcessorIndex, PGUEST_REGS GuestState, PCR3_TYPE NewCr3)
{
    CR3_TYPE ProcessCr3 = {0};

    //
    // Check if we reached to the target process or not
    //
    if ((g_ProcessSwitch.ProcessId != NULL && g_ProcessSwitch.ProcessId == PsGetCurrentProcessId()) ||
        (g_ProcessSwitch.Process != NULL && g_ProcessSwitch.Process == PsGetCurrentProcess()))
    {
        //
        // We need the kernel side cr3 of the target process
        //

        //
        // Due to KVA Shadowing, we need to switch to a different directory table base
        // if the PCID indicates this is a user mode directory table base.
        //
        NT_KPROCESS * CurrentProcess = (NT_KPROCESS *)(PsGetCurrentProcess());
        ProcessCr3.Flags             = CurrentProcess->DirectoryTableBase;

        KdHandleBreakpointAndDebugBreakpoints(ProcessorIndex, GuestState, DEBUGGEE_PAUSING_REASON_DEBUGGEE_PROCESS_SWITCHED, NULL);
    }
}

/**
 * @brief make evnvironment ready to change the process
 * @param ProcessId
 * @param EProcess
 * 
 * @return BOOLEAN 
 */
BOOLEAN
ProcessSwitch(UINT32 ProcessId, PEPROCESS EProcess)
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

    //
    // Set mov-cr3 vmexit for all the cores
    //
    CoreCount = KeQueryActiveProcessorCount(0);

    for (size_t i = 0; i < CoreCount; i++)
    {
        g_GuestState[i].DebuggingState.ThreadOrProcessTracingDetails.SetMovCr3VmExit = TRUE;
    }

    return TRUE;
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
ProcessCheckIfEprocessIsValid(UINT64 Eprocess, ULONG64 ActiveProcessHead, ULONG ActiveProcessLinksOffset)
{
    ULONG64    Process;
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
        Process = (ULONG64)ActiveProcessLinks.Flink - ActiveProcessLinksOffset;

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
            Process = (ULONG64)ActiveProcessLinks.Flink - ActiveProcessLinksOffset;

        } while ((ULONG64)ActiveProcessLinks.Flink != ActiveProcessHead);
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
 * 
 * @return BOOLEAN 
 */
BOOLEAN
ProcessShowList(PDEBUGGEE_PROCESS_LIST_NEEDED_DETAILS PorcessListSymbolInfo)
{
    ULONG64    Process;
    ULONG64    UniquePid;
    LIST_ENTRY ActiveProcessLinks;
    UCHAR      ImageFileName[15] = {0};
    CR3_TYPE   ProcessCr3        = {0};

    //
    // Set the details derived from the symbols
    //
    ULONG64 ActiveProcessHead        = PorcessListSymbolInfo->PsActiveProcessHead;      // nt!PsActiveProcessHead
    ULONG   ImageFileNameOffset      = PorcessListSymbolInfo->ImageFileNameOffset;      // nt!_EPROCESS.ImageFileName
    ULONG   UniquePidOffset          = PorcessListSymbolInfo->UniquePidOffset;          // nt!_EPROCESS.UniqueProcessId
    ULONG   ActiveProcessLinksOffset = PorcessListSymbolInfo->ActiveProcessLinksOffset; // nt!_EPROCESS.ActiveProcessLinks

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
        Process = (ULONG64)ActiveProcessLinks.Flink - ActiveProcessLinksOffset;

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
            Log("PROCESS\t%llx\n\tProcess Id: %04x\tDirBase (Kernel Cr3): %016llx\tImage: %s\n\n", Process, UniquePid, ProcessCr3.Flags, ImageFileName);

            //
            // Find the next process from the list of this process
            //
            Process = (ULONG64)ActiveProcessLinks.Flink - ActiveProcessLinksOffset;

        } while ((ULONG64)ActiveProcessLinks.Flink != ActiveProcessHead);
    }
    else
    {
        //
        // An invalid address is specified by the debugger
        //
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief change the current process
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
        if (!ProcessSwitch(PidRequest->ProcessId, PidRequest->Process))
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
        if (!ProcessShowList(&PidRequest->ProcessListSymDetails))
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
