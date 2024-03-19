/**
 * @file SwitchLayout.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Functions for switching memory layouts
 *
 * @version 0.2
 * @date 2023-04-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Switch to another process's cr3
 *
 * @details this function should NOT be called from vmx-root mode
 *
 * @param ProcessId ProcessId to switch
 * @return CR3_TYPE The cr3 of current process which can be
 * used by SwitchToPreviousProcess function
 */
_Use_decl_annotations_
CR3_TYPE
SwitchToProcessMemoryLayout(UINT32 ProcessId)
{
    UINT64    GuestCr3;
    PEPROCESS TargetEprocess;
    CR3_TYPE  CurrentProcessCr3 = {0};

    if (PsLookupProcessByProcessId((HANDLE)ProcessId, &TargetEprocess) != STATUS_SUCCESS)
    {
        //
        // There was an error, probably the process id was not found
        //
        return CurrentProcessCr3;
    }

    //
    // Due to KVA Shadowing, we need to switch to a different directory table base
    // if the PCID indicates this is a user mode directory table base.
    //
    NT_KPROCESS * CurrentProcess = (NT_KPROCESS *)(TargetEprocess);
    GuestCr3                     = CurrentProcess->DirectoryTableBase;

    //
    // Read the current cr3
    //
    CurrentProcessCr3.Flags = __readcr3();

    //
    // Change to a new cr3 (of target process)
    //
    __writecr3(GuestCr3);

    ObDereferenceObject(TargetEprocess);

    return CurrentProcessCr3;
}

/**
 * @brief Switch to guest's running process's cr3
 *
 * @details this function can be called from vmx-root mode
 *
 * @return CR3_TYPE The cr3 of current process which can be
 * used by SwitchToPreviousProcess function
 */
CR3_TYPE
SwitchToCurrentProcessMemoryLayout()
{
    CR3_TYPE GuestCr3;
    CR3_TYPE CurrentProcessCr3 = {0};

    GuestCr3.Flags = LayoutGetCurrentProcessCr3().Flags;

    //
    // Read the current cr3
    //
    CurrentProcessCr3.Flags = __readcr3();

    //
    // Change to a new cr3 (of target process)
    //
    __writecr3(GuestCr3.Flags);

    return CurrentProcessCr3;
}

/**
 * @brief Switch to another process's cr3
 *
 * @param TargetCr3 cr3 to switch
 * @return CR3_TYPE The cr3 of current process which can be
 * used by SwitchToPreviousProcess function
 */
_Use_decl_annotations_
CR3_TYPE
SwitchToProcessMemoryLayoutByCr3(CR3_TYPE TargetCr3)
{
    CR3_TYPE CurrentProcessCr3 = {0};

    //
    // Read the current cr3
    //
    CurrentProcessCr3.Flags = __readcr3();

    //
    // Change to a new cr3 (of target process)
    //
    __writecr3(TargetCr3.Flags);

    return CurrentProcessCr3;
}

/**
 * @brief Switch to previous process's cr3
 *
 * @param PreviousProcess Cr3 of previous process which
 * is returned by SwitchToProcessMemoryLayout
 * @return VOID
 */
_Use_decl_annotations_
VOID
SwitchToPreviousProcess(CR3_TYPE PreviousProcess)
{
    //
    // Restore the original cr3
    //
    __writecr3(PreviousProcess.Flags);
}
