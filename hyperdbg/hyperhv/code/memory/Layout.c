/**
 * @file Layout.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Functions for working with memory layouts
 *
 * @version 0.2
 * @date 2023-04-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Converts pid to kernel cr3
 *
 * @details this function should NOT be called from vmx-root
 *
 * @param ProcessId ProcessId to switch
 * @return CR3_TYPE The cr3 of the target process
 */
_Use_decl_annotations_
CR3_TYPE
LayoutGetCr3ByProcessId(UINT32 ProcessId)
{
    PEPROCESS TargetEprocess;
    CR3_TYPE  ProcessCr3 = {0};

    if (PsLookupProcessByProcessId((HANDLE)ProcessId, &TargetEprocess) != STATUS_SUCCESS)
    {
        //
        // There was an error, probably the process id was not found
        //
        return ProcessCr3;
    }

    //
    // Due to KVA Shadowing, we need to switch to a different directory table base
    // if the PCID indicates this is a user mode directory table base.
    //
    NT_KPROCESS * CurrentProcess = (NT_KPROCESS *)(TargetEprocess);
    ProcessCr3.Flags             = CurrentProcess->DirectoryTableBase;

    ObDereferenceObject(TargetEprocess);

    return ProcessCr3;
}

/**
 * @brief Get cr3 of the target running process
 *
 * @return CR3_TYPE Returns the cr3 of running process
 */
CR3_TYPE
LayoutGetCurrentProcessCr3()
{
    CR3_TYPE GuestCr3;

    //
    // Due to KVA Shadowing, we need to switch to a different directory table base
    // if the PCID indicates this is a user mode directory table base.
    //
    NT_KPROCESS * CurrentProcess = (NT_KPROCESS *)(PsGetCurrentProcess());
    GuestCr3.Flags               = CurrentProcess->DirectoryTableBase;

    return GuestCr3;
}

/**
 * @brief Get cr3 of the target running process
 *
 * @return CR3_TYPE Returns the cr3 of running process
 */
CR3_TYPE
LayoutGetExactGuestProcessCr3()
{
    CR3_TYPE GuestCr3 = {0};

    __vmx_vmread(VMCS_GUEST_CR3, &GuestCr3.Flags);

    return GuestCr3;
}

/**
 * @brief Find cr3 of system process
 *
 * @return UINT64 Returns cr3 of System process (pid=4)
 */
UINT64
LayoutGetSystemDirectoryTableBase()
{
    //
    // Return CR3 of the system process.
    //
    NT_KPROCESS * SystemProcess = (NT_KPROCESS *)(PsInitialSystemProcess);
    return SystemProcess->DirectoryTableBase;
}
