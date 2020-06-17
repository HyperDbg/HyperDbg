/**
 * @file Common.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Common functions that needs to be used in all source code files
 * @details
 * @version 0.1
 * @date 2020-04-10
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include <ntifs.h>
#include "Msr.h"
#include "Common.h"
#include "Vmx.h"

/**
 * @brief Power function in order to computer address for MSR bitmaps
 * 
 * @param Base Base for power function
 * @param Exp Exponent for power function
 * @return int Returns the result of power function
 */
int
MathPower(int Base, int Exp)
{
    int result;

    result = 1;

    for (;;)
    {
        if (Exp & 1)
        {
            result *= Base;
        }
        Exp >>= 1;
        if (!Exp)
        {
            break;
        }
        Base *= Base;
    }
    return result;
}

/**
 * @brief Broadcast a function to all logical cores
 * @details This function is deprecated as we want to supporrt more than 32 processors
 * 
 * @param ProcessorNumber The logical core number to execute routine on it
 * @param Routine The fucntion that should be executed on the target core
 * @return BOOLEAN Returns true if it was successfull
 */
BOOLEAN
BroadcastToProcessors(ULONG ProcessorNumber, RunOnLogicalCoreFunc Routine)
{
    KIRQL OldIrql;

    KeSetSystemAffinityThread((KAFFINITY)(1 << ProcessorNumber));

    OldIrql = KeRaiseIrqlToDpcLevel();

    Routine(ProcessorNumber);

    KeLowerIrql(OldIrql);

    KeRevertToUserAffinityThread();

    return TRUE;
}

int
TestBit(int nth, unsigned long * addr)
{
    return (BITMAP_ENTRY(nth, addr) >> BITMAP_SHIFT(nth)) & 1;
}

void
ClearBit(int nth, unsigned long * addr)
{
    BITMAP_ENTRY(nth, addr) &= ~(1UL << BITMAP_SHIFT(nth));
}

void
SetBit(int nth, unsigned long * addr)
{
    BITMAP_ENTRY(nth, addr) |= (1UL << BITMAP_SHIFT(nth));
}

/**
 * @brief Converts Virtual Address to Physical Address
 * 
 * @param VirtualAddress The target virtual address
 * @return UINT64 Returns the physical address
 */
UINT64
VirtualAddressToPhysicalAddress(PVOID VirtualAddress)
{
    return MmGetPhysicalAddress(VirtualAddress).QuadPart;
}

/**
 * @brief Switch to another process's cr3
 * 
 * @param ProcessId ProcessId to switch
 * @return CR3_TYPE The cr3 of current process which can be
 * used by RestoreToPreviousProcess function
 */
CR3_TYPE
SwitchOnAnotherProcessMemoryLayout(UINT32 ProcessId)
{
    UINT64    GuestCr3;
    PEPROCESS TargetEprocess;
    CR3_TYPE  CurrentProcessCr3 = {0};

    if (PsLookupProcessByProcessId(ProcessId, &TargetEprocess) != STATUS_SUCCESS)
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

    return CurrentProcessCr3;
}

/**
 * @brief Switch to previous process's cr3
 * 
 * @param PreviousProcess Cr3 of previous process which 
 * is returned by SwitchOnAnotherProcessMemoryLayout
 * @return VOID 
 */
VOID
RestoreToPreviousProcess(CR3_TYPE PreviousProcess)
{
    //
    // Restore the original cr3
    //
    __writecr3(PreviousProcess.Flags);
}

/**
 * @brief Converts Virtual Address to Physical Address based
 * on a specific process id's kernel cr3
 * 
 * @param VirtualAddress The target virtual address
 * @param ProcessId The target's process id
 * @return UINT64 Returns the physical address
 */
UINT64
VirtualAddressToPhysicalAddressByProcessId(PVOID VirtualAddress, UINT32 ProcessId)
{
    CR3_TYPE CurrentProcessCr3;
    UINT64   PhysicalAddress;

    //
    // Switch to new process's memory layout
    //
    CurrentProcessCr3 = SwitchOnAnotherProcessMemoryLayout(ProcessId);

    //
    // Validate if process id is valid
    //
    if (CurrentProcessCr3.Flags == NULL)
    {
        //
        // Pid is invalid
        //
        return NULL;
    }

    //
    // Read the physical address based on new cr3
    //
    PhysicalAddress = MmGetPhysicalAddress(VirtualAddress).QuadPart;

    //
    // Restore the original process
    //
    RestoreToPreviousProcess(CurrentProcessCr3);

    return PhysicalAddress;
}

/**
 * @brief Converts Physical Address to Virtual Address
 * 
 * @param PhysicalAddress The target physical address
 * @return UINT64 Returns the virtual address
 */
UINT64
PhysicalAddressToVirtualAddress(UINT64 PhysicalAddress)
{
    PHYSICAL_ADDRESS PhysicalAddr;
    PhysicalAddr.QuadPart = PhysicalAddress;

    return MmGetVirtualForPhysical(PhysicalAddr);
}

/**
 * @brief Find cr3 of system process
 * 
 * @return UINT64 Returns cr3 of System process (pid=4)
 */
UINT64
FindSystemDirectoryTableBase()
{
    //
    // Return CR3 of the system process.
    //
    NT_KPROCESS * SystemProcess = (NT_KPROCESS *)(PsInitialSystemProcess);
    return SystemProcess->DirectoryTableBase;
}
