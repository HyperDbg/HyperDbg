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

/**
 * @brief Get Index of VA on PMLx
 * 
 * @param Level PMLx
 * @param Va Virtual Address
 * @return UINT64 
 */
UINT64
MemoryMapperGetIndex(PML Level, UINT64 Va)
{
    UINT64 Result = Va;
    Result >>= 12 + Level * 9;

    return Result;
}

/**
 * @brief Get page offset
 * 
 * @param Level PMLx
 * @param Va Virtual Address
 * @return int 
 */
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
 * @param Va Virtual Address
 * @param Level PMLx
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
 * @brief This function gets virtual address and returns its PTE of the virtual address
 * based on the specific cr3
 * @details the TargetCr3 should be kernel cr3 as we will use it to translate kernel
 * addresses so the kernel functions to translate addresses should be mapped; thus,
 * don't pass a KPTI meltdown user cr3 to this function
 * 
 * @param Va Virtual Address
 * @param Level PMLx
 * @param TargetCr3 kernel cr3 of target process
 * @return PPAGE_ENTRY virtual address of PTE based on cr3
 */
PPAGE_ENTRY
MemoryMapperGetPteVaByCr3(PVOID Va, PML Level, CR3_TYPE TargetCr3)
{
    CR3_TYPE Cr3;
    CR3_TYPE CurrentProcessCr3 = {0};
    UINT64   TempCr3;
    PUINT64  Cr3Va;
    PUINT64  PdptVa;
    PUINT64  PdVa;
    PUINT64  PtVa;
    UINT32   Offset;

    //
    // Switch to new process's memory layout
    // It is because, we're not trying to change the cr3 multiple times
    // so instead of using PhysicalAddressToVirtualAddressByCr3 we use
    // PhysicalAddressToVirtualAddress, but keep in mind that cr3 should
    // be a kernel cr3 (not KPTI user cr3) as the functions to translate
    // physical address to virtual address is not mapped on the user cr3
    //
    CurrentProcessCr3 = SwitchOnAnotherProcessMemoryLayoutByCr3(TargetCr3);

    Cr3.Flags = TargetCr3.Flags;

    //
    // Cr3 should be shifted 12 to the left because it's PFN
    //
    TempCr3 = Cr3.PageFrameNumber << 12;

    //
    // we need VA of Cr3, not PA
    //
    Cr3Va = PhysicalAddressToVirtualAddress(TempCr3, TargetCr3);

    Offset = MemoryMapperGetOffset(PML4, Va);

    PPAGE_ENTRY Pml4e = &Cr3Va[Offset];

    if (!Pml4e->Present || Level == PML4)
    {
        return Pml4e;
    }

    PdptVa = PhysicalAddressToVirtualAddress(Pml4e->PageFrameNumber << 12, TargetCr3);
    Offset = MemoryMapperGetOffset(PDPT, Va);

    PPAGE_ENTRY Pdpte = &PdptVa[Offset];

    if (!Pdpte->Present || Pdpte->LargePage || Level == PDPT)
    {
        return Pdpte;
    }

    PdVa   = PhysicalAddressToVirtualAddress(Pdpte->PageFrameNumber << 12, TargetCr3);
    Offset = MemoryMapperGetOffset(PD, Va);

    PPAGE_ENTRY Pde = &PdVa[Offset];

    if (!Pde->Present || Pde->LargePage || Level == PD)
    {
        return Pde;
    }

    PtVa   = PhysicalAddressToVirtualAddress(Pde->PageFrameNumber << 12, TargetCr3);
    Offset = MemoryMapperGetOffset(PT, Va);

    PPAGE_ENTRY Pt = &PtVa[Offset];

    //
    // Restore the original process
    //
    RestoreToPreviousProcess(CurrentProcessCr3);

    return Pt;
}

/**
 * @brief This function checks if the page is mapped or not
 * @details this function checks for PRESENT Bit of the page table
 * 
 * @param Va Virtual Address
 * @param TargetCr3 kernel cr3 of target process
 * @return PPAGE_ENTRY virtual address of PTE based on cr3
 */
BOOLEAN
MemoryMapperCheckIfPageIsPresentByCr3(PVOID Va, CR3_TYPE TargetCr3)
{
    PPAGE_ENTRY PageEntry;

    //
    // Find the page table entry
    //
    PageEntry = MemoryMapperGetPteVaByCr3(Va, PT, TargetCr3);

    if (PageEntry->Present)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief This function reserve memory from system range (without physically allocating them)
 * 
 * @param Size Size of reserving buffers
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
 * 
 * @param VirtualAddress Virtual Address
 * @return VOID 
 */
VOID
MemoryMapperUnmapReservedPageRange(PVOID VirtualAddress)
{
    MmFreeMappingAddress(VirtualAddress, POOLTAG);
}

/**
 * @brief This function gets virtual address and returns its PTE (Pml4e) virtual address
 * 
 * @param VirtualAddress Virtual Address
 * @return virtual address of PTE (Pml4e)
 */
PVOID
MemoryMapperGetPte(PVOID VirtualAddress)
{
    return MemoryMapperGetPteVa(VirtualAddress, PT);
}

/**
 * @brief This function gets virtual address and returns its PTE (Pml4e) virtual address
 * based on a specific Cr3
 * 
 * @param VirtualAddress Virtual Address
 * @param TargetCr3 Target process cr3
 * @return virtual address of PTE (Pml4e)
 */
PVOID
MemoryMapperGetPteByCr3(PVOID VirtualAddress, CR3_TYPE TargetCr3)
{
    return MemoryMapperGetPteVaByCr3(VirtualAddress, PT, TargetCr3);
}

/**
 * @brief This function MAPs one resreved page (4096) and returns
 * its virtual adrresss and also PTE virtual address in PteAddress
 * 
 * @param PteAddress Address of Page Table Entry
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
 * @param SourceVA Source virtual address
 * @param PaAddressToWrite Destinaton physical address 
 * @param SizeToRead Size
 * @param PteVaAddress PTE of target virtual address
 * @param MappingVa Mapping Virtual Address
 * @param InvalidateVpids Invalidate VPIDs or not
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
 * @brief Read memory safely by mapping the buffer by physical address (It's a wrapper)
 * 
 * @param PaAddressToRead Physical Address to read
 * @param BufferToSaveMemory Destination to save 
 * @param SizeToRead Size
 * @return BOOLEAN if it was successful the returns TRUE and if it was 
 * unsuccessful then it returns FALSE
 */
BOOLEAN
MemoryMapperReadMemorySafeByPhysicalAddress(UINT64 PaAddressToRead, PVOID BufferToSaveMemory, SIZE_T SizeToRead)
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

    PhysicalAddress.QuadPart = PaAddressToRead;

    return MemoryMapperReadMemorySafeByPte(
        PhysicalAddress,
        BufferToSaveMemory,
        SizeToRead,
        g_GuestState[ProcessorIndex].MemoryMapper.PteVirtualAddress,
        g_GuestState[ProcessorIndex].MemoryMapper.VirualAddress,
        g_GuestState[ProcessorIndex].IsOnVmxRootMode);
}
/**
 * @brief Read memory safely by mapping the buffer (It's a wrapper)
 * 
 * @param VaAddressToRead Virtual Address to read
 * @param BufferToSaveMemory Destination to save 
 * @param SizeToRead Size
 * @return BOOLEAN if it was successful the returns TRUE and if it was 
 * unsuccessful then it returns FALSE
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
 * @brief Read memory safely by mapping the buffer on the target process memory (It's a wrapper)
 * 
 * @param VaAddressToRead Virtual Address to read
 * @param BufferToSaveMemory Destination to save 
 * @param SizeToRead Size
 * @return BOOLEAN if it was successful the returns TRUE and if it was 
 * unsuccessful then it returns FALSE
 */
BOOLEAN
MemoryMapperReadMemorySafeOnTargetProcess(UINT64 VaAddressToRead, PVOID BufferToSaveMemory, SIZE_T SizeToRead)
{
    CR3_TYPE GuestCr3;
    CR3_TYPE OriginalCr3;
    BOOLEAN  Result;

    //
    // Move to guest process as we're currently in system cr3
    //

    //
    // Find the current process cr3
    //
    NT_KPROCESS * CurrentProcess = (NT_KPROCESS *)(PsGetCurrentProcess());
    GuestCr3.Flags               = CurrentProcess->DirectoryTableBase;

    //
    // Move to new cr3
    //
    OriginalCr3.Flags = __readcr3();
    __writecr3(GuestCr3.Flags);

    //
    // Read target memory
    //
    Result = MemoryMapperReadMemorySafe(VaAddressToRead, BufferToSaveMemory, SizeToRead);

    //
    // Move back to original cr3
    //
    __writecr3(OriginalCr3.Flags);

    return Result;
}

/**
 * @brief Write memory safely by mapping the buffer on the target process memory (It's a wrapper)
 * 
 * @param Destination Virtual Address to write
 * @param Source value to write 
 * @param Size Size
 * @return BOOLEAN if it was successful the returns TRUE and if it was 
 * unsuccessful then it returns FALSE
 */
BOOLEAN
MemoryMapperWriteMemorySafeOnTargetProcess(UINT64 Destination, PVOID Source, SIZE_T Size)
{
    CR3_TYPE GuestCr3;
    CR3_TYPE OriginalCr3;
    BOOLEAN  Result;

    //
    // Move to guest process
    //

    //
    // Find the current process cr3
    //
    NT_KPROCESS * CurrentProcess = (NT_KPROCESS *)(PsGetCurrentProcess());
    GuestCr3.Flags               = CurrentProcess->DirectoryTableBase;

    //
    // Move to new cr3
    //
    OriginalCr3.Flags = __readcr3();
    __writecr3(GuestCr3.Flags);

    //
    // Write target memory
    //
    Result = MemoryMapperWriteMemorySafe(Destination, Source, Size, GuestCr3);

    //
    // Move back to original cr3
    //
    __writecr3(OriginalCr3.Flags);

    return Result;
}

/**
 * @brief Write memory by mapping the buffer (It's a wrapper)
 *
 * @details this function CAN be called from vmx-root mode
 * 
 * @param Destination Destination Virtual Address
 * @param Source Source Virtual Address
 * @param SizeToRead Size
 * @param TargetProcessCr3 CR3 of target process
 * 
 * @return BOOLEAN returns TRUE if it was successfull and FALSE if there was error
 */
BOOLEAN
MemoryMapperWriteMemorySafe(UINT64 Destination, PVOID Source, SIZE_T SizeToRead, CR3_TYPE TargetProcessCr3)
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

    if (TargetProcessCr3.Flags == NULL)
    {
        PhysicalAddress.QuadPart = VirtualAddressToPhysicalAddress(Destination);
    }
    else
    {
        PhysicalAddress.QuadPart = VirtualAddressToPhysicalAddressByProcessCr3(Destination, TargetProcessCr3);
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
 * @details this function should not be called from vmx-root mode
 * 
 * @param Destination Destination Virtual Address
 * @param Source Source Virtual Address
 * @param SizeToRead Size
 * @param TargetProcessId Target Process Id
 * 
 * @return BOOLEAN returns TRUE if it was successfull and FALSE if there was error
 */
BOOLEAN
MemoryMapperWriteMemoryUnsafe(UINT64 Destination, PVOID Source, SIZE_T SizeToRead, UINT32 TargetProcessId)
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
 * @param DestinationPa Destination Physical Address
 * @param Source Source Address
 * @param SizeToRead Size
 * @param TargetProcessId Target process id
 * 
 * @return BOOLEAN returns TRUE if it was successfull and FALSE if there was error 
 */
BOOLEAN
MemoryMapperWriteMemorySafeByPhysicalAddress(UINT64 DestinationPa, PVOID Source, SIZE_T SizeToRead)
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
 * @details this function should be called from vmx non-root mode
 *
 * @param ProcessId Target Process Id
 * @param Commit Whether pass the MEM_COMMIT flag to allocator or not
 * @return Reserved address in the target user mode application
 */
UINT64
MemoryMapperReserveUsermodeAddressInTargetProcess(UINT32 ProcessId, BOOLEAN Commit)
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
            DbgBreakPoint();

            //
            // Allocate (not allocate, just reserve or reserve and allocate) in memory in target process
            //
            Status = ZwAllocateVirtualMemory(
                NtCurrentProcess(),
                &AllocPtr,
                NULL,
                &AllocSize,
                Commit ? MEM_RESERVE | MEM_COMMIT : MEM_RESERVE,
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
            Commit ? MEM_RESERVE | MEM_COMMIT : MEM_RESERVE,
            PAGE_EXECUTE_READWRITE);
    }

    if (!NT_SUCCESS(Status))
    {
        return NULL;
    }

    return AllocPtr;
}

/**
 * @brief Maps a physical address to a PTE
 * @details Find the PTE from MemoryMapperGetPteVaByCr3
 * 
 * @param PhysicalAddress Physical Address
 * @param VirtualAddressPte Virtual Address of PTE
 * @param TargetKernelCr3 Target process cr3
 */
VOID
MemoryMapperMapPhysicalAddressToPte(PHYSICAL_ADDRESS PhysicalAddress, PPAGE_ENTRY VirtualAddressPte, CR3_TYPE TargetKernelCr3)
{
    PAGE_ENTRY PageEntry;
    PAGE_ENTRY PreviousPteEntry;
    CR3_TYPE   CurrentProcessCr3;

    //
    // Switch to new process's memory layout
    //
    CurrentProcessCr3 = SwitchOnAnotherProcessMemoryLayoutByCr3(TargetKernelCr3);

    //
    // Save the previous entry
    //
    PreviousPteEntry.Flags = VirtualAddressPte->Flags;

    //
    // Read the previous entry in order to modify it
    //
    PageEntry.Flags = VirtualAddressPte->Flags;

    //
    // Make sure that the target PTE is readable, writable, executable
    // present, global, etc.
    //
    PageEntry.Present = 1;

    //
    // It's not a supervisor page
    //
    PageEntry.Supervisor = 1;

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
    PageEntry.PageFrameNumber = PhysicalAddress.QuadPart >> 12;

    //
    // Apply the page entry in a single instruction
    //
    VirtualAddressPte->Flags = PageEntry.Flags;

    //
    // Finally, invalidate the caches for the virtual address.
    //
    __invlpg(PhysicalAddressToVirtualAddress(PhysicalAddress.QuadPart));

    //
    // Restore the original process
    //
    RestoreToPreviousProcess(CurrentProcessCr3);
}
