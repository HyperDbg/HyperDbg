/**
 * @file MemoryMapper.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief This file shows the functions to map memory to reserved system ranges
 * 
 * also some of the functions derived from hvpp
 * - https://github.com/wbenny/hvpp
 * 
 * @version 0.1
 * @date 2020-05-3
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

UINT64
MemoryMapperGetIndex(PML Level, UINT64 Va)
{
    UINT64 Result = Va;
    Result >>= 12 + Level * 9;

    return Result;
}

int
MemoryMapperGetOffset(PML Level, UINT64 Va)
{
    UINT64 Result = MemoryMapperGetIndex(Level, Va);
    Result &= (1 << 9) - 1; // 0x1ff

    return Result;
}

/**
 * @brief This function gets virtual address and returns its PTE of the virtual address
 * 
 * @return PPAGE_ENTRY virtual address of PTE
 */
PPAGE_ENTRY
MemoryMapperGetPteVa(PVOID Va, PML Level)
{
    CR3_TYPE Cr3;
    UINT64   TempCr3;
    PUINT64  Cr3Va;
    PUINT64  PdptVa;
    PUINT64  PdVa;
    PUINT64  PtVa;
    UINT32   Offset;

    Cr3.Flags = __readcr3();

    //
    // Cr3 should be shifted 12 to the left because it's PFN
    //
    TempCr3 = Cr3.PageFrameNumber << 12;

    //
    // we need VA of Cr3, not PA
    //
    Cr3Va = PhysicalAddressToVirtualAddress(TempCr3);

    Offset = MemoryMapperGetOffset(PML4, Va);

    PPAGE_ENTRY Pml4e = &Cr3Va[Offset];

    if (!Pml4e->Present || Level == PML4)
    {
        return Pml4e;
    }

    PdptVa = PhysicalAddressToVirtualAddress(Pml4e->PageFrameNumber << 12);
    Offset = MemoryMapperGetOffset(PDPT, Va);

    PPAGE_ENTRY Pdpte = &PdptVa[Offset];

    if (!Pdpte->Present || Pdpte->LargePage || Level == PDPT)
    {
        return Pdpte;
    }

    PdVa   = PhysicalAddressToVirtualAddress(Pdpte->PageFrameNumber << 12);
    Offset = MemoryMapperGetOffset(PD, Va);

    PPAGE_ENTRY Pde = &PdVa[Offset];

    if (!Pde->Present || Pde->LargePage || Level == PD)
    {
        return Pde;
    }

    PtVa   = PhysicalAddressToVirtualAddress(Pde->PageFrameNumber << 12);
    Offset = MemoryMapperGetOffset(PT, Va);

    PPAGE_ENTRY Pt = &PtVa[Offset];

    return Pt;
}

/**
 * @brief This function reserve memory from system range (without physically allocating them)
 * 
 * @return PVOID Return the VA of the page
 */
PVOID
MemoryMapperMapReservedPageRange(SIZE_T Size)
{
    //
    // The MmAllocateMappingAddress routine reserves a range of
    // system virtual address space of the specified size.
    //
    return MmAllocateMappingAddress(Size, POOLTAG);
}

/**
 * @brief This function frees the memory that was previously allocated
*  from system range (without physically allocating them)
 */
VOID
MemoryMapperUnmapReservedPageRange(PVOID VirtualAddress)
{
    MmFreeMappingAddress(VirtualAddress, POOLTAG);
}

/**
 * @brief This function gets virtual address and returns its PTE (Pml4e) virtual address
 * 
 * @return virtual address of PTE (Pml4e)
 */
PVOID
MemoryMapperGetPte(PVOID VirtualAddress)
{
    return MemoryMapperGetPteVa(VirtualAddress, PT);
}

/**
 * @brief This function MAPs one resreved page (4096) and returns
 * its virtual adrresss and also PTE virtual address in PteAddress
 * 
 * @return virtual address of mapped (not physically) address
 */
PVOID
MemoryMapperMapPageAndGetPte(PUINT64 PteAddress)
{
    UINT64 Va;
    UINT64 Pte;

    //
    // Reserve the page from system va space
    //
    Va = MemoryMapperMapReservedPageRange(PAGE_SIZE);

    //
    // Get the page's Page Table Entry
    //
    Pte = MemoryMapperGetPte(Va);

    *PteAddress = Pte;

    return Va;
}

/**
 * @brief Initialize the Memory Mapper
 * @details This function should be called in vmx non-root
 * in a IRQL <= APC_LEVEL
 *
 * @return VOID
 */
VOID
MemoryMapperInitialize()
{
    UINT64 TempPte;
    UINT32 ProcessorCount = KeQueryActiveProcessorCount(0);

    //
    // Reserve the address for all cores (read pte and va)
    //
    for (size_t i = 0; i < ProcessorCount; i++)
    {
        g_GuestState[i].MemoryMapper.VirualAddress     = MemoryMapperMapPageAndGetPte(&TempPte);
        g_GuestState[i].MemoryMapper.PteVirtualAddress = TempPte;
    }
}

/**
 * @brief uninitialize the Memory Mapper
 * @details This function should be called in vmx non-root
 * in a IRQL <= APC_LEVEL
 *
 * @return VOID
 */
VOID
MemoryMapperUninitialize()
{
    UINT32 ProcessorCount = KeQueryActiveProcessorCount(0);

    for (size_t i = 0; i < ProcessorCount; i++)
    {
        //
        // Unmap and free the reserved buffer
        //
        MemoryMapperUnmapReservedPageRange(g_GuestState[i].MemoryMapper.VirualAddress);
        g_GuestState[i].MemoryMapper.VirualAddress     = NULL;
        g_GuestState[i].MemoryMapper.PteVirtualAddress = NULL;
    }
}

/**
 * @brief Read memory safely by mapping the buffer using PTE
 * 
 * @return BOOLEAN returns TRUE if it was successfull and FALSE if there was error
 */
BOOLEAN
MemoryMapperReadMemorySafeByPte(PHYSICAL_ADDRESS PaAddressToRead, PVOID BufferToSaveMemory, SIZE_T SizeToRead, UINT64 PteVaAddress, UINT64 MappingVa, BOOLEAN InvalidateVpids)
{
    PVOID       Va = MappingVa;
    PVOID       NewAddress;
    PAGE_ENTRY  PageEntry;
    PPAGE_ENTRY Pte = PteVaAddress;

    //
    // Copy the previous entry into the new entry
    //
    PageEntry.Flags = Pte->Flags;

    PageEntry.Present = 1;

    //
    // Generally we want each page to be writable
    //
    PageEntry.Write = 1;

    //
    // Do not flush this page from the TLB on CR3 switch, by setting the
    // global bit in the PTE.
    //
    PageEntry.Global = 1;

    //
    // Set the PFN of this PTE to that of the provided physical address.
    //
    PageEntry.PageFrameNumber = PaAddressToRead.QuadPart >> 12;

    //
    // Apply the page entry in a single instruction
    //
    Pte->Flags = PageEntry.Flags;

    //
    // Finally, invalidate the caches for the virtual address.
    //
    __invlpg(Va);

    //
    // Also invalidate it from vpids if we're in vmx root
    //
    if (InvalidateVpids)
    {
        // __invvpid_addr(VPID_TAG, Va);
    }

    //
    // Compute the address
    //
    NewAddress = (PVOID)((UINT64)Va + (PAGE_4KB_OFFSET & (PaAddressToRead.QuadPart)));

    //
    // Move the address into the buffer in a safe manner
    //
    memcpy(BufferToSaveMemory, NewAddress, SizeToRead);

    //
    // Unmap Address
    //
    Pte->Flags = NULL;

    return TRUE;
}

/**
 * @brief Write memory safely by mapping the buffer using PTE
 * 
 * @return BOOLEAN returns TRUE if it was successfull and FALSE if there was error
 */
BOOLEAN
MemoryMapperWriteMemorySafeByPte(PVOID SourceVA, PHYSICAL_ADDRESS PaAddressToWrite, SIZE_T SizeToRead, UINT64 PteVaAddress, UINT64 MappingVa, BOOLEAN InvalidateVpids)
{
    PVOID       Va = MappingVa;
    PVOID       NewAddress;
    PAGE_ENTRY  PageEntry;
    PPAGE_ENTRY Pte = PteVaAddress;

    //
    // Copy the previous entry into the new entry
    //
    PageEntry.Flags = Pte->Flags;

    PageEntry.Present = 1;

    //
    // Generally we want each page to be writable
    //
    PageEntry.Write = 1;

    //
    // Do not flush this page from the TLB on CR3 switch, by setting the
    // global bit in the PTE.
    //
    PageEntry.Global = 1;

    //
    // Set the PFN of this PTE to that of the provided physical address.
    //
    PageEntry.PageFrameNumber = PaAddressToWrite.QuadPart >> 12;

    //
    // Apply the page entry in a single instruction
    //
    Pte->Flags = PageEntry.Flags;

    //
    // Finally, invalidate the caches for the virtual address.
    //
    __invlpg(Va);

    //
    // Also invalidate it from vpids if we're in vmx root
    //
    if (InvalidateVpids)
    {
        // __invvpid_addr(VPID_TAG, Va);
    }

    //
    // Compute the address
    //
    NewAddress = (PVOID)((UINT64)Va + (PAGE_4KB_OFFSET & (PaAddressToWrite.QuadPart)));

    //
    // Move the address into the buffer in a safe manner
    //
    memcpy(NewAddress, SourceVA, SizeToRead);

    //
    // Unmap Address
    //
    Pte->Flags = NULL;

    return TRUE;
}

/**
 * @brief Read memory safely by mapping the buffer (It's a wrapper)
 * 
 * @return BOOLEAN returns TRUE if it was successfull and FALSE if there was error
 */
BOOLEAN
MemoryMapperReadMemorySafe(UINT64 VaAddressToRead, PVOID BufferToSaveMemory, SIZE_T SizeToRead)
{
    ULONG            ProcessorIndex = KeGetCurrentProcessorNumber();
    PHYSICAL_ADDRESS PhysicalAddress;

    //
    // Check to see if PTE and Reserved VA already initialized
    //
    if (g_GuestState[ProcessorIndex].MemoryMapper.VirualAddress == NULL ||
        g_GuestState[ProcessorIndex].MemoryMapper.PteVirtualAddress == NULL)
    {
        //
        // Not initialized
        //
        return FALSE;
    }

    PhysicalAddress.QuadPart = VirtualAddressToPhysicalAddress(VaAddressToRead);

    return MemoryMapperReadMemorySafeByPte(
        PhysicalAddress,
        BufferToSaveMemory,
        SizeToRead,
        g_GuestState[ProcessorIndex].MemoryMapper.PteVirtualAddress,
        g_GuestState[ProcessorIndex].MemoryMapper.VirualAddress,
        g_GuestState[ProcessorIndex].IsOnVmxRootMode);
}

/**
 * @brief Write memory safely by mapping the buffer (It's a wrapper)
 * 
 * @return BOOLEAN returns TRUE if it was successfull and FALSE if there was error
 */
BOOLEAN
MemoryMapperWriteMemorySafe(UINT64 Destination, PVOID Source, SIZE_T SizeToRead, UINT32 TargetProcessId)
{
    ULONG            ProcessorIndex = KeGetCurrentProcessorNumber();
    PHYSICAL_ADDRESS PhysicalAddress;

    //
    // Check to see if PTE and Reserved VA already initialized
    //
    if (g_GuestState[ProcessorIndex].MemoryMapper.VirualAddress == NULL ||
        g_GuestState[ProcessorIndex].MemoryMapper.PteVirtualAddress == NULL)
    {
        //
        // Not initialized
        //
        return FALSE;
    }

    if (TargetProcessId == NULL)
    {
        PhysicalAddress.QuadPart = VirtualAddressToPhysicalAddress(Destination);
    }
    else
    {
        PhysicalAddress.QuadPart = VirtualAddressToPhysicalAddressByProcessId(Destination, TargetProcessId);
    }

    return MemoryMapperWriteMemorySafeByPte(
        Source,
        PhysicalAddress,
        SizeToRead,
        g_GuestState[ProcessorIndex].MemoryMapper.PteVirtualAddress,
        g_GuestState[ProcessorIndex].MemoryMapper.VirualAddress,
        g_GuestState[ProcessorIndex].IsOnVmxRootMode);
}

/**
 * @brief Write memory safely by mapping the buffer (It's a wrapper)
 * 
 * @return BOOLEAN returns TRUE if it was successfull and FALSE if there was error
 */
BOOLEAN
MemoryMapperWriteMemorySafeByPhysicalAddress(UINT64 DestinationPa, PVOID Source, SIZE_T SizeToRead, UINT32 TargetProcessId)
{
    ULONG            ProcessorIndex = KeGetCurrentProcessorNumber();
    PHYSICAL_ADDRESS PhysicalAddress;

    //
    // Check to see if PTE and Reserved VA already initialized
    //
    if (g_GuestState[ProcessorIndex].MemoryMapper.VirualAddress == NULL ||
        g_GuestState[ProcessorIndex].MemoryMapper.PteVirtualAddress == NULL)
    {
        //
        // Not initialized
        //
        return FALSE;
    }

    PhysicalAddress.QuadPart = DestinationPa;

    return MemoryMapperWriteMemorySafeByPte(
        Source,
        PhysicalAddress,
        SizeToRead,
        g_GuestState[ProcessorIndex].MemoryMapper.PteVirtualAddress,
        g_GuestState[ProcessorIndex].MemoryMapper.VirualAddress,
        g_GuestState[ProcessorIndex].IsOnVmxRootMode);
}

/**
 * @brief Reserve user mode address (not allocated) in the target user mode application
 * 
 * @return Reserved address in the target user mode application
 */
UINT64
MemoryMapperReserveUsermodeAddressInTargetProcess(UINT32 ProcessId)
{
    NTSTATUS   Status;
    PVOID      AllocPtr  = NULL;
    SIZE_T     AllocSize = PAGE_SIZE;
    PEPROCESS  SourceProcess;
    KAPC_STATE State = {0};

    if (PsGetCurrentProcessId() != ProcessId)
    {
        //
        // User needs another process memory
        //

        if (PsLookupProcessByProcessId(ProcessId, &SourceProcess) != STATUS_SUCCESS)
        {
            //
            // if the process not found
            //
            return STATUS_UNSUCCESSFUL;
        }
        __try
        {
            KeStackAttachProcess(SourceProcess, &State);

            //
            // Allocate (not allocate, just reserve) in memory in target process
            //
            Status = ZwAllocateVirtualMemory(
                NtCurrentProcess(),
                &AllocPtr,
                NULL,
                &AllocSize,
                MEM_RESERVE,
                PAGE_EXECUTE_READWRITE);

            KeUnstackDetachProcess(&State);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            KeUnstackDetachProcess(&State);
            return STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        //
        // Allocate (not allocate, just reserve) in memory in target process
        //
        Status = ZwAllocateVirtualMemory(
            NtCurrentProcess(),
            &AllocPtr,
            NULL,
            &AllocSize,
            MEM_RESERVE,
            PAGE_EXECUTE_READWRITE);
    }

    if (!NT_SUCCESS(Status))
    {
        return NULL;
    }

    return AllocPtr;
}
