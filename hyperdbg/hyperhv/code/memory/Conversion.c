/**
 * @file Conversion.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Functions for memory conversions
 *
 * @version 0.2
 * @date 2023-04-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Converts Physical Address to Virtual Address
 *
 * @param PhysicalAddress The target physical address
 * @return UINT64 Returns the virtual address
 */
_Use_decl_annotations_
UINT64
PhysicalAddressToVirtualAddress(UINT64 PhysicalAddress)
{
    PHYSICAL_ADDRESS PhysicalAddr;
    PhysicalAddr.QuadPart = PhysicalAddress;

    return (UINT64)MmGetVirtualForPhysical(PhysicalAddr);
}

/**
 * @brief Converts Physical Address to Virtual Address based
 * on a specific process id
 *
 * @details this function should NOT be called from vmx-root mode
 *
 * @param PhysicalAddress The target physical address
 * @param ProcessId The target's process id
 * @return UINT64 Returns the virtual address
 */
_Use_decl_annotations_
UINT64
PhysicalAddressToVirtualAddressByProcessId(PVOID PhysicalAddress, UINT32 ProcessId)
{
    CR3_TYPE         CurrentProcessCr3;
    UINT64           VirtualAddress;
    PHYSICAL_ADDRESS PhysicalAddr;

    //
    // Switch to new process's memory layout
    //
    CurrentProcessCr3 = SwitchToProcessMemoryLayout(ProcessId);

    //
    // Validate if process id is valid
    //
    if (CurrentProcessCr3.Flags == NULL64_ZERO)
    {
        //
        // Pid is invalid
        //
        return NULL64_ZERO;
    }

    //
    // Read the virtual address based on new cr3
    //
    PhysicalAddr.QuadPart = (LONGLONG)PhysicalAddress;
    VirtualAddress        = (UINT64)MmGetVirtualForPhysical(PhysicalAddr);

    //
    // Restore the original process
    //
    SwitchToPreviousProcess(CurrentProcessCr3);

    return VirtualAddress;
}

/**
 * @brief Converts Physical Address to Virtual Address based
 * on a specific process's kernel cr3
 *
 * @details this function should NOT be called from vmx-root mode
 *
 * @param PhysicalAddress The target physical address
 * @param TargetCr3 The target's process cr3
 * @return UINT64 Returns the virtual address
 */
_Use_decl_annotations_
UINT64
PhysicalAddressToVirtualAddressByCr3(PVOID PhysicalAddress, CR3_TYPE TargetCr3)
{
    CR3_TYPE         CurrentProcessCr3;
    UINT64           VirtualAddress;
    PHYSICAL_ADDRESS PhysicalAddr;

    //
    // Switch to new process's memory layout
    //
    CurrentProcessCr3 = SwitchToProcessMemoryLayoutByCr3(TargetCr3);

    //
    // Validate if process id is valid
    //
    if (CurrentProcessCr3.Flags == NULL64_ZERO)
    {
        //
        // Pid is invalid
        //
        return NULL64_ZERO;
    }

    //
    // Read the virtual address based on new cr3
    //
    PhysicalAddr.QuadPart = (LONGLONG)PhysicalAddress;
    VirtualAddress        = (UINT64)MmGetVirtualForPhysical(PhysicalAddr);

    //
    // Restore the original process
    //
    SwitchToPreviousProcess(CurrentProcessCr3);

    return VirtualAddress;
}

/**
 * @brief Converts Physical Address to Virtual Address based
 * on current process's kernel cr3
 *
 * @details this function should NOT be called from vmx-root mode
 *
 * @param PhysicalAddress The target physical address
 * @return UINT64 Returns the virtual address
 */
_Use_decl_annotations_
UINT64
PhysicalAddressToVirtualAddressOnTargetProcess(PVOID PhysicalAddress)
{
    CR3_TYPE GuestCr3;

    GuestCr3.Flags = LayoutGetCurrentProcessCr3().Flags;

    return PhysicalAddressToVirtualAddressByCr3(PhysicalAddress, GuestCr3);
}

/**
 * @brief Converts Virtual Address to Physical Address
 *
 * @param VirtualAddress The target virtual address
 * @return UINT64 Returns the physical address
 */
_Use_decl_annotations_
UINT64
VirtualAddressToPhysicalAddress(_In_ PVOID VirtualAddress)
{
    return MmGetPhysicalAddress(VirtualAddress).QuadPart;
}

/**
 * @brief Converts Virtual Address to Physical Address based
 * on a specific process id's kernel cr3
 *
 * @details this function should NOT be called from vmx-root mode
 *
 * @param VirtualAddress The target virtual address
 * @param ProcessId The target's process id
 * @return UINT64 Returns the physical address
 */
_Use_decl_annotations_
UINT64
VirtualAddressToPhysicalAddressByProcessId(PVOID VirtualAddress, UINT32 ProcessId)
{
    CR3_TYPE CurrentProcessCr3;
    UINT64   PhysicalAddress;

    //
    // Switch to new process's memory layout
    //
    CurrentProcessCr3 = SwitchToProcessMemoryLayout(ProcessId);

    //
    // Validate if process id is valid
    //
    if (CurrentProcessCr3.Flags == NULL64_ZERO)
    {
        //
        // Pid is invalid
        //
        return NULL64_ZERO;
    }

    //
    // Read the physical address based on new cr3
    //
    PhysicalAddress = MmGetPhysicalAddress(VirtualAddress).QuadPart;

    //
    // Restore the original process
    //
    SwitchToPreviousProcess(CurrentProcessCr3);

    return PhysicalAddress;
}

/**
 * @brief Converts Virtual Address to Physical Address based
 * on a specific process's kernel cr3
 *
 * @param VirtualAddress The target virtual address
 * @param TargetCr3 The target's process cr3
 * @return UINT64 Returns the physical address
 */
_Use_decl_annotations_
UINT64
VirtualAddressToPhysicalAddressByProcessCr3(PVOID VirtualAddress, CR3_TYPE TargetCr3)
{
    CR3_TYPE CurrentProcessCr3;
    UINT64   PhysicalAddress;

    //
    // Switch to new process's memory layout
    //
    CurrentProcessCr3 = SwitchToProcessMemoryLayoutByCr3(TargetCr3);

    //
    // Validate if process id is valid
    //
    if (CurrentProcessCr3.Flags == NULL64_ZERO)
    {
        //
        // Pid is invalid
        //
        return NULL64_ZERO;
    }

    //
    // Read the physical address based on new cr3
    //
    PhysicalAddress = MmGetPhysicalAddress(VirtualAddress).QuadPart;

    //
    // Restore the original process
    //
    SwitchToPreviousProcess(CurrentProcessCr3);

    return PhysicalAddress;
}

/**
 * @brief Converts Virtual Address to Physical Address based
 * on the current process's kernel cr3
 *
 * @param VirtualAddress The target virtual address
 * @return UINT64 Returns the physical address
 */
_Use_decl_annotations_
UINT64
VirtualAddressToPhysicalAddressOnTargetProcess(PVOID VirtualAddress)
{
    CR3_TYPE CurrentCr3;
    CR3_TYPE GuestCr3;
    UINT64   PhysicalAddress;

    GuestCr3.Flags = LayoutGetCurrentProcessCr3().Flags;

    //
    // Switch to new process's memory layout
    //
    CurrentCr3 = SwitchToProcessMemoryLayoutByCr3(GuestCr3);

    //
    // Validate if process id is valid
    //
    if (CurrentCr3.Flags == NULL64_ZERO)
    {
        //
        // Pid is invalid
        //
        return NULL64_ZERO;
    }

    //
    // Read the physical address based on new cr3
    //
    PhysicalAddress = MmGetPhysicalAddress(VirtualAddress).QuadPart;

    //
    // Restore the original process
    //
    SwitchToPreviousProcess(CurrentCr3);

    return PhysicalAddress;
}
