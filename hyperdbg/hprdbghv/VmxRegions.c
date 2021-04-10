/**
 * @file VmxRegions.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Implement allocations for VMX Regions (VMXON Region, VMCS, MSR Bitmap and etc.)
 * @details
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief Allocates Vmx regions for all logical cores (Vmxon region and Vmcs region)
 * 
 * @param Dpc 
 * @param DeferredContext 
 * @param SystemArgument1 
 * @param SystemArgument2 
 * @return BOOLEAN
 */
BOOLEAN
VmxDpcBroadcastAllocateVmxonRegions(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    int CurrentProcessorNumber = KeGetCurrentProcessorNumber();

    LogDebugInfo("Allocating Vmx Regions for logical core %d", CurrentProcessorNumber);

    //
    // Enabling VMX Operation
    //
    AsmEnableVmxOperation();

    LogDebugInfo("VMX-Operation Enabled Successfully");

    if (!VmxAllocateVmxonRegion(&g_GuestState[CurrentProcessorNumber]))
    {
        LogError("Error in allocating memory for Vmxon region");
        return FALSE;
    }
    if (!VmxAllocateVmcsRegion(&g_GuestState[CurrentProcessorNumber]))
    {
        LogError("Error in allocating memory for Vmcs region");
        return FALSE;
    }

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);

    return TRUE;
}

/**
 * @brief Allocates Vmxon region and set the Revision ID based on IA32_VMX_BASIC_MSR
 * 
 * @param CurrentGuestState 
 * @return BOOLEAN Returns true if allocation was successfull and vmxon executed without error
 * otherwise returns false
 */
BOOLEAN
VmxAllocateVmxonRegion(VIRTUAL_MACHINE_STATE * CurrentGuestState)
{
    PHYSICAL_ADDRESS   PhysicalMax = {0};
    IA32_VMX_BASIC_MSR VmxBasicMsr = {0};
    int                VmxonSize;
    int                VmxonStatus;
    BYTE *             VmxonRegion;
    UINT64             VmxonRegionPhysicalAddr;
    UINT64             AlignedVmxonRegion;
    UINT64             AlignedVmxonRegionPhysicalAddr;

    //
    // at IRQL > DISPATCH_LEVEL memory allocation routines don't work
    //
    if (KeGetCurrentIrql() > DISPATCH_LEVEL)
        KeRaiseIrqlToDpcLevel();

    PhysicalMax.QuadPart = MAXULONG64;

    VmxonSize = 2 * VMXON_SIZE;

    //
    // Allocating a 4-KByte Contigous Memory region
    //
    VmxonRegion = MmAllocateContiguousMemory(VmxonSize + ALIGNMENT_PAGE_SIZE, PhysicalMax);

    if (VmxonRegion == NULL)
    {
        LogError("Couldn't Allocate Buffer for VMXON Region.");
        return FALSE;
    }

    VmxonRegionPhysicalAddr = VirtualAddressToPhysicalAddress(VmxonRegion);

    //
    // zero-out memory
    //
    RtlSecureZeroMemory(VmxonRegion, VmxonSize + ALIGNMENT_PAGE_SIZE);

    AlignedVmxonRegion = (BYTE *)((ULONG_PTR)(VmxonRegion + ALIGNMENT_PAGE_SIZE - 1) & ~(ALIGNMENT_PAGE_SIZE - 1));
    LogDebugInfo("VMXON Region Address : %llx", AlignedVmxonRegion);

    //
    // 4 kb >= buffers are aligned, just a double check to ensure if it's aligned
    //
    AlignedVmxonRegionPhysicalAddr = (BYTE *)((ULONG_PTR)(VmxonRegionPhysicalAddr + ALIGNMENT_PAGE_SIZE - 1) & ~(ALIGNMENT_PAGE_SIZE - 1));
    LogDebugInfo("VMXON Region Physical Address : %llx", AlignedVmxonRegionPhysicalAddr);

    //
    // get IA32_VMX_BASIC_MSR RevisionId
    //
    VmxBasicMsr.All = __readmsr(MSR_IA32_VMX_BASIC);
    LogDebugInfo("Revision Identifier (MSR_IA32_VMX_BASIC - MSR 0x480) : 0x%x", VmxBasicMsr.Fields.RevisionIdentifier);

    //
    //Changing Revision Identifier
    //
    *(UINT64 *)AlignedVmxonRegion = VmxBasicMsr.Fields.RevisionIdentifier;

    //
    // Execute Vmxon instruction
    //
    VmxonStatus = __vmx_on(&AlignedVmxonRegionPhysicalAddr);
    if (VmxonStatus)
    {
        LogError("Executing Vmxon instruction failed with status : %d", VmxonStatus);
        return FALSE;
    }

    CurrentGuestState->VmxonRegionPhysicalAddress = AlignedVmxonRegionPhysicalAddr;

    //
    // We save the allocated buffer (not the aligned buffer) because we want to free it in vmx termination
    //
    CurrentGuestState->VmxonRegionVirtualAddress = VmxonRegion;

    return TRUE;
}

/**
 * @brief Allocate Vmcs region and set the Revision ID based on IA32_VMX_BASIC_MSR
 * 
 * @param CurrentGuestState 
 * @return BOOLEAN Returns true if allocation was successfull and vmptrld executed without error
 * otherwise returns false
 */
BOOLEAN
VmxAllocateVmcsRegion(VIRTUAL_MACHINE_STATE * CurrentGuestState)
{
    PHYSICAL_ADDRESS   PhysicalMax = {0};
    int                VmcsSize;
    BYTE *             VmcsRegion;
    UINT64             VmcsPhysicalAddr;
    UINT64             AlignedVmcsRegion;
    UINT64             AlignedVmcsRegionPhysicalAddr;
    IA32_VMX_BASIC_MSR VmxBasicMsr = {0};

    //
    // at IRQL > DISPATCH_LEVEL memory allocation routines don't work
    //
    if (KeGetCurrentIrql() > DISPATCH_LEVEL)
        KeRaiseIrqlToDpcLevel();

    PhysicalMax.QuadPart = MAXULONG64;

    VmcsSize   = 2 * VMCS_SIZE;
    VmcsRegion = MmAllocateContiguousMemory(VmcsSize + ALIGNMENT_PAGE_SIZE, PhysicalMax); // Allocating a 4-KByte Contigous Memory region

    if (VmcsRegion == NULL)
    {
        LogError("Couldn't Allocate Buffer for VMCS Region.");
        return FALSE;
    }
    RtlSecureZeroMemory(VmcsRegion, VmcsSize + ALIGNMENT_PAGE_SIZE);

    VmcsPhysicalAddr = VirtualAddressToPhysicalAddress(VmcsRegion);

    AlignedVmcsRegion = (BYTE *)((ULONG_PTR)(VmcsRegion + ALIGNMENT_PAGE_SIZE - 1) & ~(ALIGNMENT_PAGE_SIZE - 1));
    LogDebugInfo("VMCS Region Address : %llx", AlignedVmcsRegion);

    AlignedVmcsRegionPhysicalAddr = (BYTE *)((ULONG_PTR)(VmcsPhysicalAddr + ALIGNMENT_PAGE_SIZE - 1) & ~(ALIGNMENT_PAGE_SIZE - 1));
    LogDebugInfo("VMCS Region Physical Address : %llx", AlignedVmcsRegionPhysicalAddr);

    //
    // get IA32_VMX_BASIC_MSR RevisionId
    //
    VmxBasicMsr.All = __readmsr(MSR_IA32_VMX_BASIC);
    LogDebugInfo("Revision Identifier (MSR_IA32_VMX_BASIC - MSR 0x480) : 0x%x", VmxBasicMsr.Fields.RevisionIdentifier);

    //
    //Changing Revision Identifier
    //
    *(UINT64 *)AlignedVmcsRegion = VmxBasicMsr.Fields.RevisionIdentifier;

    CurrentGuestState->VmcsRegionPhysicalAddress = AlignedVmcsRegionPhysicalAddr;
    //
    // We save the allocated buffer (not the aligned buffer)
    // because we want to free it in vmx termination
    //
    CurrentGuestState->VmcsRegionVirtualAddress = VmcsRegion;

    return TRUE;
}

/**
 * @brief Allocate VMM Stack
 * 
 * @param ProcessorID Logical Core Id
 * @return BOOLEAN Returns true if allocation was successfull otherwise returns false
 */
BOOLEAN
VmxAllocateVmmStack(INT ProcessorID)
{
    UINT64 VmmStack;

    //
    // Allocate stack for the VM Exit Handler
    //
    VmmStack                           = ExAllocatePoolWithTag(NonPagedPool, VMM_STACK_SIZE, POOLTAG);
    g_GuestState[ProcessorID].VmmStack = VmmStack;

    if (g_GuestState[ProcessorID].VmmStack == NULL)
    {
        LogError("Insufficient memory in allocationg Vmm stack");
        return FALSE;
    }
    RtlZeroMemory(g_GuestState[ProcessorID].VmmStack, VMM_STACK_SIZE);

    LogDebugInfo("Vmm Stack for logical processor : 0x%llx", g_GuestState[ProcessorID].VmmStack);

    return TRUE;
}

/**
 * @brief Allocate a buffer forr Msr Bitmap
 * 
 * @param ProcessorID 
 * @return BOOLEAN Returns true if allocation was successfull otherwise returns false
 */
BOOLEAN
VmxAllocateMsrBitmap(INT ProcessorID)
{
    //
    // Allocate memory for MSR Bitmap
    //
    g_GuestState[ProcessorID].MsrBitmapVirtualAddress = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOLTAG); // should be aligned

    if (g_GuestState[ProcessorID].MsrBitmapVirtualAddress == NULL)
    {
        LogError("Insufficient memory in allocationg Msr Bitmaps");
        return FALSE;
    }
    RtlZeroMemory(g_GuestState[ProcessorID].MsrBitmapVirtualAddress, PAGE_SIZE);

    g_GuestState[ProcessorID].MsrBitmapPhysicalAddress = VirtualAddressToPhysicalAddress(g_GuestState[ProcessorID].MsrBitmapVirtualAddress);

    LogDebugInfo("Msr Bitmap Virtual Address : 0x%llx", g_GuestState[ProcessorID].MsrBitmapVirtualAddress);
    LogDebugInfo("Msr Bitmap Physical Address : 0x%llx", g_GuestState[ProcessorID].MsrBitmapPhysicalAddress);

    return TRUE;
}

/**
 * @brief Allocate a buffer forr I/O Bitmap
 * 
 * @param ProcessorID 
 * @return BOOLEAN Returns true if allocation was successfull otherwise returns false
 */
BOOLEAN
VmxAllocateIoBitmaps(INT ProcessorID)
{
    //
    // Allocate memory for I/O Bitmap (A)
    //
    g_GuestState[ProcessorID].IoBitmapVirtualAddressA = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOLTAG); // should be aligned

    if (g_GuestState[ProcessorID].IoBitmapVirtualAddressA == NULL)
    {
        LogError("Insufficient memory in allocationg I/O Bitmaps A");
        return FALSE;
    }
    RtlZeroMemory(g_GuestState[ProcessorID].IoBitmapVirtualAddressA, PAGE_SIZE);

    g_GuestState[ProcessorID].IoBitmapPhysicalAddressA = VirtualAddressToPhysicalAddress(g_GuestState[ProcessorID].IoBitmapVirtualAddressA);

    LogDebugInfo("I/O Bitmap A Virtual Address : 0x%llx", g_GuestState[ProcessorID].IoBitmapVirtualAddressA);
    LogDebugInfo("I/O Bitmap A Physical Address : 0x%llx", g_GuestState[ProcessorID].IoBitmapPhysicalAddressA);

    //
    // Allocate memory for I/O Bitmap (B)
    //
    g_GuestState[ProcessorID].IoBitmapVirtualAddressB = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOLTAG); // should be aligned

    if (g_GuestState[ProcessorID].IoBitmapVirtualAddressB == NULL)
    {
        LogError("Insufficient memory in allocationg I/O Bitmaps B");
        return FALSE;
    }
    RtlZeroMemory(g_GuestState[ProcessorID].IoBitmapVirtualAddressB, PAGE_SIZE);

    g_GuestState[ProcessorID].IoBitmapPhysicalAddressB = VirtualAddressToPhysicalAddress(g_GuestState[ProcessorID].IoBitmapVirtualAddressB);

    LogDebugInfo("I/O Bitmap B Virtual Address : 0x%llx", g_GuestState[ProcessorID].IoBitmapVirtualAddressB);
    LogDebugInfo("I/O Bitmap B Physical Address : 0x%llx", g_GuestState[ProcessorID].IoBitmapPhysicalAddressB);

    return TRUE;
}
