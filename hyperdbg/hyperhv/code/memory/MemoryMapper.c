/**
 * @file MemoryMapper.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief This file shows the functions to map memory to reserved system ranges
 * @details also some of the functions derived from hvpp
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
_Use_decl_annotations_
UINT64
MemoryMapperGetIndex(PAGING_LEVEL Level, UINT64 Va)
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
 * @return UINT32
 */
_Use_decl_annotations_
UINT32
MemoryMapperGetOffset(PAGING_LEVEL Level, UINT64 Va)
{
    UINT32 Result = (UINT32)MemoryMapperGetIndex(Level, Va);
    Result &= (1 << 9) - 1; // 0x1ff

    return Result;
}

/**
 * @brief This function gets virtual address and returns its PTE of the virtual address
 *
 * @param Va Virtual Address
 * @param Level PMLx
 * @return PVOID virtual address of PTE
 */
_Use_decl_annotations_
PVOID
MemoryMapperGetPteVa(PVOID Va, PAGING_LEVEL Level)
{
    CR3_TYPE Cr3;

    //
    // Read the current cr3
    //
    Cr3.Flags = __readcr3();

    //
    // Call the wrapper
    //
    return MemoryMapperGetPteVaWithoutSwitchingByCr3(Va, Level, Cr3);
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
 * @return PVOID virtual address of PTE based on cr3
 */
_Use_decl_annotations_
PVOID
MemoryMapperGetPteVaByCr3(PVOID Va, PAGING_LEVEL Level, CR3_TYPE TargetCr3)
{
    PPAGE_ENTRY PageEntry         = NULL;
    CR3_TYPE    CurrentProcessCr3 = {0};

    //
    // Switch to new process's memory layout
    // It is because, we're not trying to change the cr3 multiple times
    // so instead of using PhysicalAddressToVirtualAddressByCr3 we use
    // PhysicalAddressToVirtualAddress, but keep in mind that cr3 should
    // be a kernel cr3 (not KPTI user cr3) as the functions to translate
    // physical address to virtual address is not mapped on the user cr3
    //
    CurrentProcessCr3 = SwitchToProcessMemoryLayoutByCr3(TargetCr3);

    //
    // Call the wrapper
    //
    PageEntry = MemoryMapperGetPteVaWithoutSwitchingByCr3(Va, Level, TargetCr3);

    //
    // Restore the original process
    //
    SwitchToPreviousProcess(CurrentProcessCr3);

    return PageEntry;
}

/**
 * @brief This function gets virtual address and returns its PTE of the virtual address
 * based on the target virtual address
 *
 * @param Va Virtual Address
 * @param Level PMLx
 *
 * @return PVOID virtual address of PTE based on cr3
 */
_Use_decl_annotations_
PVOID
MemoryMapperGetPteVaOnTargetProcess(PVOID Va, PAGING_LEVEL Level)
{
    PPAGE_ENTRY PageEntry = NULL;
    CR3_TYPE    GuestCr3;
    CR3_TYPE    CurrentProcessCr3 = {0};

    //
    // Move to guest process as we're currently in system cr3
    //

    //
    // Find the current process cr3
    //
    GuestCr3.Flags = LayoutGetCurrentProcessCr3().Flags;

    //
    // Switch to new process's memory layout
    // It is because, we're not trying to change the cr3 multiple times
    // so instead of using PhysicalAddressToVirtualAddressByCr3 we use
    // PhysicalAddressToVirtualAddress, but keep in mind that cr3 should
    // be a kernel cr3 (not KPTI user cr3) as the functions to translate
    // physical address to virtual address is not mapped on the user cr3
    //
    CurrentProcessCr3 = SwitchToProcessMemoryLayoutByCr3(GuestCr3);

    //
    // Call the wrapper
    //
    PageEntry = MemoryMapperGetPteVaWithoutSwitchingByCr3(Va, Level, GuestCr3);

    //
    // Restore the original process
    //
    SwitchToPreviousProcess(CurrentProcessCr3);

    return PageEntry;
}

/**
 * @brief This function checks whether the virtual address is present in the RAM or not
 *
 * @param Va Virtual Address
 * @param Level PMLx
 *
 * @return BOOLEAN Is present or not
 */
BOOLEAN
MemoryMapperCheckPteIsPresentOnTargetProcess(PVOID Va, PAGING_LEVEL Level)
{
    PPAGE_ENTRY PageEntry = NULL;
    CR3_TYPE    GuestCr3;
    CR3_TYPE    CurrentProcessCr3 = {0};
    BOOLEAN     Result            = FALSE;

    //
    // Move to guest process as we're currently in system cr3
    //

    //
    // Find the current process cr3
    //
    GuestCr3.Flags = LayoutGetCurrentProcessCr3().Flags;

    //
    // Switch to new process's memory layout
    // It is because, we're not trying to change the cr3 multiple times
    // so instead of using PhysicalAddressToVirtualAddressByCr3 we use
    // PhysicalAddressToVirtualAddress, but keep in mind that cr3 should
    // be a kernel cr3 (not KPTI user cr3) as the functions to translate
    // physical address to virtual address is not mapped on the user cr3
    //
    CurrentProcessCr3 = SwitchToProcessMemoryLayoutByCr3(GuestCr3);

    //
    // Call the wrapper
    //
    PageEntry = MemoryMapperGetPteVaWithoutSwitchingByCr3(Va, Level, GuestCr3);

    if (PageEntry == NULL)
    {
        Result = FALSE;
    }
    else
    {
        //
        // Check if page is present or not
        //
        if (PageEntry->Fields.Present == TRUE)
        {
            //
            // It's present
            //
            Result = TRUE;
        }
        else
        {
            //
            // It's not present
            //
            Result = FALSE;
        }
    }

    //
    // Restore the original process
    //
    SwitchToPreviousProcess(CurrentProcessCr3);

    return Result;
}

/**
 * @brief This function gets virtual address and returns its PTE of the virtual address
 * based on the target virtual address
 * @details the TargetCr3 should be kernel cr3 as we will use it to translate kernel
 * addresses so the kernel functions to translate addresses should be mapped; thus,
 * don't pass a KPTI meltdown user cr3 to this function
 *
 * @param Va Virtual Address
 * @param Set
 *
 * @return PVOID virtual address of PTE based on cr3
 */
_Use_decl_annotations_
PVOID
MemoryMapperSetExecuteDisableToPteOnTargetProcess(PVOID Va, BOOLEAN Set)
{
    PPAGE_ENTRY PageEntry = NULL;
    CR3_TYPE    GuestCr3;
    CR3_TYPE    CurrentProcessCr3 = {0};

    //
    // Move to guest process as we're currently in system cr3
    //

    //
    // Find the current process cr3
    //
    GuestCr3.Flags = LayoutGetCurrentProcessCr3().Flags;

    //
    // Switch to new process's memory layout
    // It is because, we're not trying to change the cr3 multiple times
    // so instead of using PhysicalAddressToVirtualAddressByCr3 we use
    // PhysicalAddressToVirtualAddress, but keep in mind that cr3 should
    // be a kernel cr3 (not KPTI user cr3) as the functions to translate
    // physical address to virtual address is not mapped on the user cr3
    //
    CurrentProcessCr3 = SwitchToProcessMemoryLayoutByCr3(GuestCr3);

    //
    // Call the wrapper
    //
    PageEntry = MemoryMapperGetPteVaWithoutSwitchingByCr3(Va, PagingLevelPageTable, GuestCr3);

    //
    // Set execute disable bit
    //
    PageEntry->Fields.ExecuteDisable = Set;

    //
    // Invalidate the TLB
    //
    __invlpg(Va);

    //
    // Restore the original process
    //
    SwitchToPreviousProcess(CurrentProcessCr3);

    return PageEntry;
}

/**
 * @brief This function gets virtual address and returns its PTE of the virtual address
 * based on the specific cr3 but without switching to the target address
 * @details the TargetCr3 should be kernel cr3 as we will use it to translate kernel
 * addresses so the kernel functions to translate addresses should be mapped; thus,
 * don't pass a KPTI meltdown user cr3 to this function
 *
 * @param Va Virtual Address
 * @param Level PMLx
 * @param TargetCr3 kernel cr3 of target process
 * @return PVOID virtual address of PTE based on cr3
 */
_Use_decl_annotations_
PVOID
MemoryMapperGetPteVaWithoutSwitchingByCr3(PVOID Va, PAGING_LEVEL Level, CR3_TYPE TargetCr3)
{
    CR3_TYPE Cr3;
    UINT64   TempCr3;
    PUINT64  Cr3Va;
    PUINT64  PdptVa;
    PUINT64  PdVa;
    PUINT64  PtVa;
    UINT32   Offset;

    Cr3.Flags = TargetCr3.Flags;

    //
    // Cr3 should be shifted 12 to the left because it's PFN
    //
    TempCr3 = Cr3.Fields.PageFrameNumber << 12;

    //
    // we need VA of Cr3, not PA
    //
    Cr3Va = (UINT64 *)PhysicalAddressToVirtualAddress(TempCr3);

    //
    // Check for invalid address
    //
    if (Cr3Va == NULL)
    {
        return NULL;
    }

    Offset = MemoryMapperGetOffset(PagingLevelPageMapLevel4, (UINT64)Va);

    PPAGE_ENTRY Pml4e = (PAGE_ENTRY *)&Cr3Va[Offset];

    if (!Pml4e->Fields.Present || Level == PagingLevelPageMapLevel4)
    {
        return Pml4e;
    }

    PdptVa = (UINT64 *)PhysicalAddressToVirtualAddress(Pml4e->Fields.PageFrameNumber << 12);

    //
    // Check for invalid address
    //
    if (PdptVa == NULL)
    {
        return NULL;
    }

    Offset = MemoryMapperGetOffset(PagingLevelPageDirectoryPointerTable, (UINT64)Va);

    PPAGE_ENTRY Pdpte = (PAGE_ENTRY *)&PdptVa[Offset];

    if (!Pdpte->Fields.Present || Pdpte->Fields.LargePage || Level == PagingLevelPageDirectoryPointerTable)
    {
        return Pdpte;
    }

    PdVa = (UINT64 *)PhysicalAddressToVirtualAddress(Pdpte->Fields.PageFrameNumber << 12);

    //
    // Check for invalid address
    //
    if (PdVa == NULL)
    {
        return NULL;
    }

    Offset = MemoryMapperGetOffset(PagingLevelPageDirectory, (UINT64)Va);

    PPAGE_ENTRY Pde = (PAGE_ENTRY *)&PdVa[Offset];

    if (!Pde->Fields.Present || Pde->Fields.LargePage || Level == PagingLevelPageDirectory)
    {
        return Pde;
    }

    PtVa = (UINT64 *)PhysicalAddressToVirtualAddress(Pde->Fields.PageFrameNumber << 12);

    //
    // Check for invalid address
    //
    if (PtVa == NULL)
    {
        return NULL;
    }

    Offset = MemoryMapperGetOffset(PagingLevelPageTable, (UINT64)Va);

    PPAGE_ENTRY Pt = (PAGE_ENTRY *)&PtVa[Offset];

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
_Use_decl_annotations_
BOOLEAN
MemoryMapperCheckIfPageIsPresentByCr3(PVOID Va, CR3_TYPE TargetCr3)
{
    PPAGE_ENTRY PageEntry;

    //
    // Find the page table entry
    //
    PageEntry = MemoryMapperGetPteVaByCr3(Va, PagingLevelPageTable, TargetCr3);

    if (PageEntry != NULL && PageEntry->Fields.Present)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief This function checks if the page has NX bit or not
 *
 * @param Va Virtual Address
 * @param TargetCr3 kernel cr3 of target process
 * @return PPAGE_ENTRY virtual address of PTE based on cr3
 */
_Use_decl_annotations_
BOOLEAN
MemoryMapperCheckIfPageIsNxBitSetByCr3(PVOID Va, CR3_TYPE TargetCr3)
{
    PPAGE_ENTRY PageEntry;

    //
    // Find the page table entry
    //
    PageEntry = MemoryMapperGetPteVaByCr3(Va, PagingLevelPageTable, TargetCr3);

    if (PageEntry != NULL && !PageEntry->Fields.ExecuteDisable)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief This function checks target process to see
 * if the page has NX bit or not
 *
 * @param Va Virtual Address
 * @param TargetCr3 kernel cr3 of target process
 * @return BOOLEAN
 */
_Use_decl_annotations_
BOOLEAN
MemoryMapperCheckIfPageIsNxBitSetOnTargetProcess(PVOID Va)
{
    BOOLEAN     Result;
    CR3_TYPE    GuestCr3;
    PPAGE_ENTRY PageEntry;
    CR3_TYPE    CurrentProcessCr3 = {0};

    //
    // Move to guest process as we're currently in system cr3
    //

    //
    // Find the current process cr3
    //
    GuestCr3.Flags = LayoutGetCurrentProcessCr3().Flags;

    CurrentProcessCr3 = SwitchToProcessMemoryLayoutByCr3(GuestCr3);

    //
    // Find the page table entry
    //
    PageEntry = MemoryMapperGetPteVa(Va, PagingLevelPageTable);

    if (PageEntry != NULL && !PageEntry->Fields.ExecuteDisable)
    {
        Result = TRUE;
    }
    else
    {
        Result = FALSE;
    }

    //
    // Restore the original process
    //
    SwitchToPreviousProcess(CurrentProcessCr3);

    return Result;
}

/**
 * @brief This function checks target process to see
 * if the PDE is a large page or not
 *
 * @param Va Virtual Address
 * @param TargetCr3 kernel cr3 of target process
 * @return BOOLEAN
 */
_Use_decl_annotations_
BOOLEAN
MemoryMapperCheckIfPdeIsLargePageOnTargetProcess(PVOID Va)
{
    BOOLEAN     Result;
    CR3_TYPE    GuestCr3;
    PPAGE_ENTRY PageEntry;
    CR3_TYPE    CurrentProcessCr3 = {0};

    //
    // Move to guest process as we're currently in system cr3
    //

    //
    // Find the current process cr3
    //
    GuestCr3.Flags = LayoutGetCurrentProcessCr3().Flags;

    CurrentProcessCr3 = SwitchToProcessMemoryLayoutByCr3(GuestCr3);

    //
    // Find the page table entry (PDE)
    //
    PageEntry = MemoryMapperGetPteVa(Va, PagingLevelPageDirectory);

    if (PageEntry != NULL && PageEntry->Fields.LargePage)
    {
        Result = TRUE;
    }
    else
    {
        Result = FALSE;
    }

    //
    // Restore the original process
    //
    SwitchToPreviousProcess(CurrentProcessCr3);

    return Result;
}

/**
 * @brief This function reserve memory from system range (without physically allocating them)
 *
 * @param Size Size of reserving buffers
 * @return PVOID Return the VA of the page
 */
_Use_decl_annotations_
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
_Use_decl_annotations_
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
_Use_decl_annotations_
PVOID
MemoryMapperGetPte(UINT64 VirtualAddress)
{
    return MemoryMapperGetPteVa((PVOID)VirtualAddress, PagingLevelPageTable);
}

/**
 * @brief This function gets virtual address and returns its PTE (Pml4e) virtual address
 * based on a specific Cr3
 *
 * @param VirtualAddress Virtual Address
 * @param TargetCr3 Target process cr3
 * @return virtual address of PTE (Pml4e)
 */
_Use_decl_annotations_
PVOID
MemoryMapperGetPteByCr3(PVOID VirtualAddress, CR3_TYPE TargetCr3)
{
    return MemoryMapperGetPteVaByCr3(VirtualAddress, PagingLevelPageTable, TargetCr3);
}

/**
 * @brief This function MAPs resreved pages (4096) and returns its virtual adrresss
 *
 * @param ReserveSize Size of the reserved page
 *
 * @return virtual address of mapped (not physically) address
 */
_Use_decl_annotations_
PVOID
MemoryMapperMapPageAndGetPte(UINT32 ReserveSize)
{
    PVOID Va;

    //
    // Reserve the page from system va space
    //
    Va = MemoryMapperMapReservedPageRange(ReserveSize);

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
    ULONG ProcessorsCount;

    ProcessorsCount = KeQueryActiveProcessorCount(0);

    //
    // *** Reserve the address for all cores (read pte and va) ***
    //

    if (g_MemoryMapper != NULL)
    {
        //
        // It's already initialized
        //
        return;
    }

    //
    // Allocate the memory buffer structure
    //
    g_MemoryMapper = PlatformMemAllocateZeroedNonPagedPool(sizeof(MEMORY_MAPPER_ADDRESSES) * ProcessorsCount);

    //
    // Set the core's id and initialize memory mapper
    //
    for (size_t i = 0; i < ProcessorsCount; i++)
    {
        //
        // *** Initialize memory mapper for each core ***
        //

        //
        // Initial and reserve for read operations
        //
        g_MemoryMapper[i].VirtualAddressForRead    = (UINT64)MemoryMapperMapPageAndGetPte(PAGE_SIZE);
        g_MemoryMapper[i].PteVirtualAddressForRead = (UINT64)MemoryMapperGetPte(g_MemoryMapper[i].VirtualAddressForRead);

        //
        // Initial and reserve for write operations
        //
        g_MemoryMapper[i].VirtualAddressForWrite    = (UINT64)MemoryMapperMapPageAndGetPte(PAGE_SIZE);
        g_MemoryMapper[i].PteVirtualAddressForWrite = (UINT64)MemoryMapperGetPte(g_MemoryMapper[i].PteVirtualAddressForWrite);

        //
        // Initial and reserve for MMIO operations
        //
        g_MemoryMapper[i].VirtualAddressForMmioAccess     = (UINT64)MemoryMapperMapPageAndGetPte(PAGE_SIZE * 2);
        g_MemoryMapper[i].PteVirtualAddressForMmioAccess1 = (UINT64)MemoryMapperGetPte(g_MemoryMapper[i].VirtualAddressForMmioAccess);
        g_MemoryMapper[i].PteVirtualAddressForMmioAccess2 = (UINT64)MemoryMapperGetPte((UINT64)g_MemoryMapper[i].VirtualAddressForMmioAccess + PAGE_SIZE);

        // LogInfo("MMIO Address for core %d: %llx, PTE1: %llx, PTE2: %llx",
        //         i,
        //         g_MemoryMapper[i].VirtualAddressForMmioAccess,
        //         g_MemoryMapper[i].PteVirtualAddressForMmioAccess1,
        //         g_MemoryMapper[i].PteVirtualAddressForMmioAccess2);
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
    ULONG ProcessorsCount = KeQueryActiveProcessorCount(0);

    for (size_t i = 0; i < ProcessorsCount; i++)
    {
        //
        // Unmap and free the reserved buffer
        //
        if (g_MemoryMapper[i].VirtualAddressForRead != NULL64_ZERO)
        {
            MemoryMapperUnmapReservedPageRange((PVOID)g_MemoryMapper[i].VirtualAddressForRead);
        }

        if (g_MemoryMapper[i].VirtualAddressForWrite != NULL64_ZERO)
        {
            MemoryMapperUnmapReservedPageRange((PVOID)g_MemoryMapper[i].VirtualAddressForWrite);
        }

        if (g_MemoryMapper[i].VirtualAddressForMmioAccess != NULL64_ZERO)
        {
            MemoryMapperUnmapReservedPageRange((PVOID)g_MemoryMapper[i].VirtualAddressForMmioAccess);
        }

        g_MemoryMapper[i].VirtualAddressForRead    = NULL64_ZERO;
        g_MemoryMapper[i].PteVirtualAddressForRead = NULL64_ZERO;

        g_MemoryMapper[i].VirtualAddressForWrite    = NULL64_ZERO;
        g_MemoryMapper[i].PteVirtualAddressForWrite = NULL64_ZERO;

        g_MemoryMapper[i].PteVirtualAddressForMmioAccess1 = NULL64_ZERO;
        g_MemoryMapper[i].PteVirtualAddressForMmioAccess2 = NULL64_ZERO;
        g_MemoryMapper[i].VirtualAddressForMmioAccess     = NULL64_ZERO;
    }

    //
    // Set the g_MemoryMapper to null
    //
    g_MemoryMapper = NULL;
}

/**
 * @brief Read memory safely by mapping the buffer using PTE
 * @param PaAddressToRead Physical address to read
 * @param BufferToSaveMemory buffer to save the memory
 * @param SizeToRead Size
 * @param PteVaAddress Virtual Address of PTE
 * @param MappingVa Mapping virtual address
 * @param InvalidateVpids whether invalidate based on VPIDs or not
 *
 * @return BOOLEAN returns TRUE if it was successful and FALSE if there was error
 */
_Use_decl_annotations_
BOOLEAN
MemoryMapperReadMemorySafeByPte(PHYSICAL_ADDRESS PaAddressToRead,
                                PVOID            BufferToSaveMemory,
                                SIZE_T           SizeToRead,
                                UINT64           PteVaAddress,
                                UINT64           MappingVa,
                                BOOLEAN          InvalidateVpids)
{
    PVOID       NewAddress;
    PAGE_ENTRY  PageEntry;
    PPAGE_ENTRY Pte = (PAGE_ENTRY *)PteVaAddress;
    PVOID       Va  = (PVOID)MappingVa;

    //
    // Copy the previous entry into the new entry
    //
    PageEntry.Flags = Pte->Flags;

    PageEntry.Fields.Present = 1;

    //
    // Generally we want each page to be writable
    //
    PageEntry.Fields.Write = 1;

    //
    // Do not flush this page from the TLB on CR3 switch, by setting the
    // global bit in the PTE.
    //
    PageEntry.Fields.Global = 1;

    //
    // Set the PFN of this PTE to that of the provided physical address,
    //
    PageEntry.Fields.PageFrameNumber = PaAddressToRead.QuadPart >> 12;

    //
    // Apply the page entry in a single instruction
    //
    Pte->Flags = PageEntry.Flags;

    //
    // Finally, invalidate the caches for the virtual address
    // It's not mandatory to invalidate the address in the VM nested-virtualization
    // because it will be automatically invalidated by the top hypervisor, however,
    // we should use invlpg in physical computers as it won't invalidate it automatically
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
    Pte->Flags = NULL64_ZERO;

    return TRUE;
}

/**
 * @brief Write memory safely by mapping the buffer using PTE
 *
 * @param SourceVA Source virtual address
 * @param PaAddressToWrite Destination physical address
 * @param SizeToWrite Size
 * @param PteVaAddress PTE of target virtual address
 * @param MappingVa Mapping Virtual Address
 * @param InvalidateVpids Invalidate VPIDs or not
 * @return BOOLEAN returns TRUE if it was successful and FALSE if there was error
 */
_Use_decl_annotations_
BOOLEAN
MemoryMapperWriteMemorySafeByPte(PVOID            SourceVA,
                                 PHYSICAL_ADDRESS PaAddressToWrite,
                                 SIZE_T           SizeToWrite,
                                 UINT64           PteVaAddress,
                                 UINT64           MappingVa,
                                 BOOLEAN          InvalidateVpids)
{
    PVOID       NewAddress;
    PAGE_ENTRY  PageEntry;
    PPAGE_ENTRY Pte = (PAGE_ENTRY *)PteVaAddress;
    PVOID       Va  = (PVOID)MappingVa;

    //
    // Copy the previous entry into the new entry
    //
    PageEntry.Flags = Pte->Flags;

    PageEntry.Fields.Present = 1;

    //
    // Generally we want each page to be writable
    //
    PageEntry.Fields.Write = 1;

    //
    // Do not flush this page from the TLB on CR3 switch, by setting the
    // global bit in the PTE.
    //
    PageEntry.Fields.Global = 1;

    //
    // Set the PFN of this PTE to that of the provided physical address.
    //
    PageEntry.Fields.PageFrameNumber = PaAddressToWrite.QuadPart >> 12;

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
    memcpy(NewAddress, SourceVA, SizeToWrite);

    //
    // Unmap Address
    //
    Pte->Flags = NULL64_ZERO;

    return TRUE;
}

/**
 * @brief Wrapper to read the memory safely by mapping the
 * buffer by physical address (It's a wrapper)
 *
 * @param TypeOfRead Type of read
 * @param AddressToRead Physical Address to read
 * @return UINT64 returns the target physical address and NULL if it fails
 */
_Use_decl_annotations_
UINT64
MemoryMapperReadMemorySafeByPhysicalAddressWrapperAddressMaker(
    MEMORY_MAPPER_WRAPPER_FOR_MEMORY_READ TypeOfRead,
    UINT64                                AddressToRead)
{
    PHYSICAL_ADDRESS PhysicalAddress = {0};

    switch (TypeOfRead)
    {
    case MEMORY_MAPPER_WRAPPER_READ_PHYSICAL_MEMORY:

        PhysicalAddress.QuadPart = AddressToRead;

        break;

    case MEMORY_MAPPER_WRAPPER_READ_VIRTUAL_MEMORY:

        PhysicalAddress.QuadPart = VirtualAddressToPhysicalAddress((PVOID)AddressToRead);

        break;

    default:

        return NULL64_ZERO;
        break;
    }

    return PhysicalAddress.QuadPart;
}

/**
 * @brief Wrapper to read the memory safely by mapping the
 * buffer by physical address (It's a wrapper)
 *
 * @param TypeOfRead Type of read
 * @param AddressToRead Address to read
 * @param BufferToSaveMemory Destination to save
 * @param SizeToRead Size
 * @return BOOLEAN if it was successful the returns TRUE and if it was
 * unsuccessful then it returns FALSE
 */
_Use_decl_annotations_
BOOLEAN
MemoryMapperReadMemorySafeByPhysicalAddressWrapper(
    MEMORY_MAPPER_WRAPPER_FOR_MEMORY_READ TypeOfRead,
    UINT64                                AddressToRead,
    UINT64                                BufferToSaveMemory,
    SIZE_T                                SizeToRead)
{
    ULONG            CurrentCore = KeGetCurrentProcessorNumberEx(NULL);
    UINT64           AddressToCheck;
    PHYSICAL_ADDRESS PhysicalAddress;

    //
    // Check to see if PTE and Reserved VA already initialized
    //
    if (g_MemoryMapper[CurrentCore].VirtualAddressForRead == NULL64_ZERO ||
        g_MemoryMapper[CurrentCore].PteVirtualAddressForRead == NULL64_ZERO)
    {
        //
        // Not initialized
        //
        return FALSE;
    }

    //
    // Check whether we should apply multiple accesses or not
    //
    AddressToCheck = (CHAR *)AddressToRead + SizeToRead - ((CHAR *)PAGE_ALIGN(AddressToRead));

    if (AddressToCheck > PAGE_SIZE)
    {
        //
        // Address should be accessed in more than one page
        //
        UINT64 ReadSize = AddressToCheck;

        while (SizeToRead != 0)
        {
            ReadSize = (UINT64)PAGE_ALIGN(AddressToRead + PAGE_SIZE) - AddressToRead;

            if (ReadSize == PAGE_SIZE && SizeToRead < PAGE_SIZE)
            {
                ReadSize = SizeToRead;
            }

            /*
            LogInfo("Addr From : %llx to Addr To : %llx | ReadSize : %llx\n",
                    AddressToRead,
                    AddressToRead + ReadSize,
                    ReadSize);
            */

            //
            // One access is enough (page+size won't pass from the PAGE_ALIGN boundary)
            //
            PhysicalAddress.QuadPart = MemoryMapperReadMemorySafeByPhysicalAddressWrapperAddressMaker(TypeOfRead,
                                                                                                      AddressToRead);

            if (!MemoryMapperReadMemorySafeByPte(
                    PhysicalAddress,
                    (PVOID)BufferToSaveMemory,
                    ReadSize,
                    g_MemoryMapper[CurrentCore].PteVirtualAddressForRead,
                    g_MemoryMapper[CurrentCore].VirtualAddressForRead,
                    FALSE))
            {
                return FALSE;
            }

            //
            // Apply the changes to the next addresses (if any)
            //
            SizeToRead         = SizeToRead - ReadSize;
            AddressToRead      = AddressToRead + ReadSize;
            BufferToSaveMemory = BufferToSaveMemory + ReadSize;
        }

        return TRUE;
    }
    else
    {
        //
        // One access is enough (page+size won't pass from the PAGE_ALIGN boundary)
        //
        PhysicalAddress.QuadPart = MemoryMapperReadMemorySafeByPhysicalAddressWrapperAddressMaker(TypeOfRead,
                                                                                                  AddressToRead);

        return MemoryMapperReadMemorySafeByPte(
            PhysicalAddress,
            (PVOID)BufferToSaveMemory,
            SizeToRead,
            g_MemoryMapper[CurrentCore].PteVirtualAddressForRead,
            g_MemoryMapper[CurrentCore].VirtualAddressForRead,
            FALSE);
    }
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
_Use_decl_annotations_
BOOLEAN
MemoryMapperReadMemorySafeByPhysicalAddress(UINT64 PaAddressToRead,
                                            UINT64 BufferToSaveMemory,
                                            SIZE_T SizeToRead)
{
    //
    // Call the wrapper
    //
    return MemoryMapperReadMemorySafeByPhysicalAddressWrapper(MEMORY_MAPPER_WRAPPER_READ_PHYSICAL_MEMORY,
                                                              PaAddressToRead,
                                                              BufferToSaveMemory,
                                                              SizeToRead);
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
_Use_decl_annotations_
BOOLEAN
MemoryMapperReadMemorySafe(UINT64 VaAddressToRead, PVOID BufferToSaveMemory, SIZE_T SizeToRead)
{
    return MemoryMapperReadMemorySafeByPhysicalAddressWrapper(MEMORY_MAPPER_WRAPPER_READ_VIRTUAL_MEMORY,
                                                              VaAddressToRead,
                                                              (UINT64)BufferToSaveMemory,
                                                              SizeToRead);
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
_Use_decl_annotations_
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
    GuestCr3.Flags = LayoutGetCurrentProcessCr3().Flags;

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
_Use_decl_annotations_
BOOLEAN
MemoryMapperWriteMemorySafeOnTargetProcess(UINT64 Destination, PVOID Source, SIZE_T Size)
{
    CR3_TYPE GuestCr3;
    CR3_TYPE OriginalCr3;
    BOOLEAN  Result;

    //
    // *** Move to guest process ***
    //

    //
    // Find the current process cr3
    //
    GuestCr3.Flags = LayoutGetCurrentProcessCr3().Flags;

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
 * @brief Decides about making the address and converting the address
 * to physical address based on the passed parameters
 *
 * @param TypeOfWrite Type of memory write
 * @param DestinationAddr Destination Address
 * @param TargetProcessCr3 The process CR3 (might be null)
 * @param TargetProcessId The process PID (might be null)
 *
 * @return UINT64 returns the target physical address and NULL if it fails
 */
_Use_decl_annotations_
UINT64
MemoryMapperWriteMemorySafeWrapperAddressMaker(MEMORY_MAPPER_WRAPPER_FOR_MEMORY_WRITE TypeOfWrite,
                                               UINT64                                 DestinationAddr,
                                               PCR3_TYPE                              TargetProcessCr3,
                                               UINT32                                 TargetProcessId)
{
    PHYSICAL_ADDRESS PhysicalAddress = {0};

    switch (TypeOfWrite)
    {
    case MEMORY_MAPPER_WRAPPER_WRITE_PHYSICAL_MEMORY:

        PhysicalAddress.QuadPart = DestinationAddr;

        break;

    case MEMORY_MAPPER_WRAPPER_WRITE_VIRTUAL_MEMORY_UNSAFE:

        if (TargetProcessId == NULL_ZERO)
        {
            PhysicalAddress.QuadPart = VirtualAddressToPhysicalAddress((PVOID)DestinationAddr);
        }
        else
        {
            PhysicalAddress.QuadPart = VirtualAddressToPhysicalAddressByProcessId((PVOID)DestinationAddr, TargetProcessId);
        }

        break;

    case MEMORY_MAPPER_WRAPPER_WRITE_VIRTUAL_MEMORY_SAFE:

        if (TargetProcessCr3 == NULL || TargetProcessCr3->Flags == NULL64_ZERO)
        {
            PhysicalAddress.QuadPart = VirtualAddressToPhysicalAddress((PVOID)DestinationAddr);
        }
        else
        {
            PhysicalAddress.QuadPart = VirtualAddressToPhysicalAddressByProcessCr3((PVOID)DestinationAddr, *TargetProcessCr3);
        }

        break;

    default:
        return NULL64_ZERO;

        break;
    }

    return PhysicalAddress.QuadPart;
}

/**
 * @brief Write memory safely by mapping the buffer (It's a wrapper)
 *
 * @param TypeOfWrite Type of memory write
 * @param DestinationAddr Destination Address
 * @param Source Source Address
 * @param SizeToWrite Size
 * @param TargetProcessCr3 The process CR3 (might be null)
 * @param TargetProcessId The process PID (might be null)
 *
 * @return BOOLEAN returns TRUE if it was successful and FALSE if there was error
 */
_Use_decl_annotations_
BOOLEAN
MemoryMapperWriteMemorySafeWrapper(MEMORY_MAPPER_WRAPPER_FOR_MEMORY_WRITE TypeOfWrite,
                                   UINT64                                 DestinationAddr,
                                   UINT64                                 Source,
                                   SIZE_T                                 SizeToWrite,
                                   PCR3_TYPE                              TargetProcessCr3,
                                   UINT32                                 TargetProcessId)
{
    ULONG            CurrentCore = KeGetCurrentProcessorNumberEx(NULL);
    UINT64           AddressToCheck;
    PHYSICAL_ADDRESS PhysicalAddress;

    //
    // Check to see if PTE and Reserved VA already initialized
    //
    if (g_MemoryMapper[CurrentCore].VirtualAddressForWrite == NULL64_ZERO ||
        g_MemoryMapper[CurrentCore].PteVirtualAddressForWrite == NULL64_ZERO)
    {
        //
        // Not initialized
        //
        return FALSE;
    }

    //
    // Check whether it needs multiple accesses to different pages or no
    //
    AddressToCheck = (CHAR *)DestinationAddr + SizeToWrite - ((CHAR *)PAGE_ALIGN(DestinationAddr));

    if (AddressToCheck > PAGE_SIZE)
    {
        //
        // It need multiple accesses to different pages to access the memory
        //

        UINT64 WriteSize = AddressToCheck;

        while (SizeToWrite != 0)
        {
            WriteSize = (UINT64)PAGE_ALIGN(DestinationAddr + PAGE_SIZE) - DestinationAddr;

            if (WriteSize == PAGE_SIZE && SizeToWrite < PAGE_SIZE)
            {
                WriteSize = SizeToWrite;
            }

            /*
            LogInfo("Addr From : %llx to Addr To : %llx | WriteSize : %llx\n",
                    DestinationAddr,
                    DestinationAddr + WriteSize,
                    WriteSize);
            */

            PhysicalAddress.QuadPart = MemoryMapperWriteMemorySafeWrapperAddressMaker(TypeOfWrite,
                                                                                      DestinationAddr,
                                                                                      TargetProcessCr3,
                                                                                      TargetProcessId);

            if (!MemoryMapperWriteMemorySafeByPte(
                    (PVOID)Source,
                    PhysicalAddress,
                    WriteSize,
                    g_MemoryMapper[CurrentCore].PteVirtualAddressForWrite,
                    g_MemoryMapper[CurrentCore].VirtualAddressForWrite,
                    FALSE))
            {
                return FALSE;
            }

            SizeToWrite     = SizeToWrite - WriteSize;
            DestinationAddr = DestinationAddr + WriteSize;
            Source          = Source + WriteSize;
        }

        return TRUE;
    }
    else
    {
        //
        // One access is enough to write
        //
        PhysicalAddress.QuadPart = MemoryMapperWriteMemorySafeWrapperAddressMaker(TypeOfWrite,
                                                                                  DestinationAddr,
                                                                                  TargetProcessCr3,
                                                                                  TargetProcessId);
        return MemoryMapperWriteMemorySafeByPte(
            (PVOID)Source,
            PhysicalAddress,
            SizeToWrite,
            g_MemoryMapper[CurrentCore].PteVirtualAddressForWrite,
            g_MemoryMapper[CurrentCore].VirtualAddressForWrite,
            FALSE);
    }
}

/**
 * @brief Write memory by mapping the buffer (It's a wrapper)
 *
 * @details this function CAN be called from vmx-root mode
 *
 * @param Destination Destination Virtual Address
 * @param Source Source Virtual Address
 * @param SizeToWrite Size
 * @param TargetProcessCr3 CR3 of target process
 *
 * @return BOOLEAN returns TRUE if it was successful and FALSE if there was error
 */
_Use_decl_annotations_
BOOLEAN
MemoryMapperWriteMemorySafe(UINT64   Destination,
                            PVOID    Source,
                            SIZE_T   SizeToWrite,
                            CR3_TYPE TargetProcessCr3)
{
    return MemoryMapperWriteMemorySafeWrapper(MEMORY_MAPPER_WRAPPER_WRITE_VIRTUAL_MEMORY_SAFE,
                                              Destination,
                                              (UINT64)Source,
                                              SizeToWrite,
                                              &TargetProcessCr3,
                                              NULL_ZERO);
}

/**
 * @brief Write memory safely by mapping the buffer (It's a wrapper)
 *
 * @details this function should not be called from vmx-root mode
 *
 * @param Destination Destination Virtual Address
 * @param Source Source Virtual Address
 * @param SizeToWrite Size
 * @param TargetProcessId Target Process Id
 *
 * @return BOOLEAN returns TRUE if it was successful and FALSE if there was error
 */
_Use_decl_annotations_
BOOLEAN
MemoryMapperWriteMemoryUnsafe(UINT64 Destination, PVOID Source, SIZE_T SizeToWrite, UINT32 TargetProcessId)
{
    return MemoryMapperWriteMemorySafeWrapper(MEMORY_MAPPER_WRAPPER_WRITE_VIRTUAL_MEMORY_UNSAFE,
                                              Destination,
                                              (UINT64)Source,
                                              SizeToWrite,
                                              NULL,
                                              TargetProcessId);
}

/**
 * @brief Write memory safely by mapping the buffer
 *
 * @param DestinationPa Destination Physical Address
 * @param Source Source Address
 * @param SizeToWrite Size
 *
 * @return BOOLEAN returns TRUE if it was successful and FALSE if there was error
 */
_Use_decl_annotations_
BOOLEAN
MemoryMapperWriteMemorySafeByPhysicalAddress(UINT64 DestinationPa,
                                             UINT64 Source,
                                             SIZE_T SizeToWrite)
{
    //
    // Call the wrapper for safe memory read
    //
    return MemoryMapperWriteMemorySafeWrapper(MEMORY_MAPPER_WRAPPER_WRITE_PHYSICAL_MEMORY,
                                              DestinationPa,
                                              Source,
                                              SizeToWrite,
                                              NULL,
                                              NULL_ZERO);
}

/**
 * @brief Reserve user mode address (not allocated) in the target user mode application
 * @details this function should be called from vmx non-root mode
 *
 * @param ProcessId Target Process Id
 * @param Allocate Whether allocate or just reserve
 * @return Reserved address in the target user mode application
 */
_Use_decl_annotations_
UINT64
MemoryMapperReserveUsermodeAddressOnTargetProcess(UINT32 ProcessId, BOOLEAN Allocate)
{
    NTSTATUS   Status;
    PVOID      AllocPtr  = NULL;
    SIZE_T     AllocSize = PAGE_SIZE;
    PEPROCESS  SourceProcess;
    KAPC_STATE State = {0};

    if (PsGetCurrentProcessId() != (HANDLE)ProcessId)
    {
        //
        // User needs another process memory
        //

        if (PsLookupProcessByProcessId((HANDLE)ProcessId, &SourceProcess) != STATUS_SUCCESS)
        {
            //
            // if the process not found
            //
            return NULL64_ZERO;
        }
        __try
        {
            KeStackAttachProcess(SourceProcess, &State);

            //
            // Allocate (not allocate, just reserve or reserve and allocate) in memory in target process
            //
            Status = ZwAllocateVirtualMemory(
                NtCurrentProcess(),
                &AllocPtr,
                (ULONG_PTR)NULL,
                &AllocSize,
                Allocate ? MEM_COMMIT : MEM_RESERVE,
                PAGE_EXECUTE_READWRITE);

            KeUnstackDetachProcess(&State);

            ObDereferenceObject(SourceProcess);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            KeUnstackDetachProcess(&State);

            ObDereferenceObject(SourceProcess);
            return NULL64_ZERO;
        }
    }
    else
    {
        //
        // Allocate in memory in target process
        //
        Status = ZwAllocateVirtualMemory(
            NtCurrentProcess(),
            &AllocPtr,
            (ULONG_PTR)NULL,
            &AllocSize,
            Allocate ? MEM_COMMIT : MEM_RESERVE,
            PAGE_EXECUTE_READWRITE);
    }

    if (!NT_SUCCESS(Status))
    {
        return NULL64_ZERO;
    }

    return (UINT64)AllocPtr;
}

/**
 * @brief Deallocates a previously reserved user mode address in the target user mode application
 * @details this function should be called from vmx non-root mode
 *
 * @param ProcessId Target Process Id
 * @param BaseAddress Previously allocated base address
 * @return BOOLEAN whether the operation was successful or not
 */
_Use_decl_annotations_
BOOLEAN
MemoryMapperFreeMemoryOnTargetProcess(UINT32 ProcessId,
                                      PVOID  BaseAddress)
{
    NTSTATUS   Status;
    SIZE_T     AllocSize = PAGE_SIZE;
    PEPROCESS  SourceProcess;
    KAPC_STATE State = {0};

    if (PsGetCurrentProcessId() != (HANDLE)ProcessId)
    {
        //
        // User needs another process memory
        //

        if (PsLookupProcessByProcessId((HANDLE)ProcessId, &SourceProcess) != STATUS_SUCCESS)
        {
            //
            // if the process not found
            //
            return FALSE;
        }
        __try
        {
            KeStackAttachProcess(SourceProcess, &State);

            //
            // Free memory in target process
            //
            Status = ZwFreeVirtualMemory(NtCurrentProcess(),
                                         &BaseAddress,
                                         &AllocSize,
                                         MEM_RELEASE);

            KeUnstackDetachProcess(&State);

            ObDereferenceObject(SourceProcess);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            KeUnstackDetachProcess(&State);

            ObDereferenceObject(SourceProcess);
            return FALSE;
        }
    }
    else
    {
        //
        // Deallocate memory in target process
        //
        Status = ZwFreeVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     &AllocSize,
                                     MEM_RELEASE);
    }

    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    //
    // Operation was successful
    //
    return TRUE;
}

/**
 * @brief Maps a physical address to a PTE
 * @details Find the PTE from MemoryMapperGetPteVaByCr3
 *
 * @param PhysicalAddress Physical Address to be mapped
 * @param TargetProcessVirtualAddress Virtual Address of target process
 * @param TargetProcessKernelCr3 Target process cr3
 *
 * @return VOID
 */
_Use_decl_annotations_
VOID
MemoryMapperMapPhysicalAddressToPte(PHYSICAL_ADDRESS PhysicalAddress,
                                    PVOID            TargetProcessVirtualAddress,
                                    CR3_TYPE         TargetProcessKernelCr3)
{
    PPAGE_ENTRY PreviousPteEntry;
    PAGE_ENTRY  PageEntry;
    CR3_TYPE    CurrentProcessCr3;

    //
    // Find the page table entry of the reserved page in the target
    // process memory layout
    //
    PreviousPteEntry = MemoryMapperGetPteVaByCr3(TargetProcessVirtualAddress, PagingLevelPageTable, TargetProcessKernelCr3);

    //
    // Switch to new process's memory layout
    //
    CurrentProcessCr3 = SwitchToProcessMemoryLayoutByCr3(TargetProcessKernelCr3);

    //
    // Read the previous entry in order to modify it
    //
    PageEntry.Flags = PreviousPteEntry->Flags;

    //
    // Make sure that the target PTE is readable, writable, executable
    // present, global, etc.
    //
    PageEntry.Fields.Present = 1;

    //
    // It's not a supervisor page
    //
    PageEntry.Fields.Supervisor = 1;

    //
    // Generally we want each page to be writable
    //
    PageEntry.Fields.Write = 1;

    //
    // Do not flush this page from the TLB on CR3 switch, by setting the
    // global bit in the PTE.
    //
    PageEntry.Fields.Global = 1;

    //
    // Set the PFN of this PTE to that of the provided physical address.
    //
    PageEntry.Fields.PageFrameNumber = PhysicalAddress.QuadPart >> 12;

    //
    // Apply the page entry in a single instruction
    //
    PreviousPteEntry->Flags = PageEntry.Flags;

    //
    // Finally, invalidate the caches for the virtual address
    // It's not mandatory to invalidate the address in the VM nested-virtualization
    // because it will be automatically invalidated by the top hypervisor, however,
    // we should use invlpg in physical computers as it won't invalidate it automatically
    //
    __invlpg(TargetProcessVirtualAddress);

    //
    // Restore the original process
    //
    SwitchToPreviousProcess(CurrentProcessCr3);
}

/**
 * @brief This function the Supervisor bit of the target PTE based on the specific cr3
 *
 * @param Va Virtual Address
 * @param Set Set it to 1 or 0
 * @param Level PMLx
 * @param TargetCr3 kernel cr3 of target process
 * @return BOOLEAN whether was successful or not
 */
_Use_decl_annotations_
BOOLEAN
MemoryMapperSetSupervisorBitWithoutSwitchingByCr3(PVOID Va, BOOLEAN Set, PAGING_LEVEL Level, CR3_TYPE TargetCr3)
{
    PPAGE_ENTRY Pml = NULL;

    Pml = MemoryMapperGetPteVaWithoutSwitchingByCr3(Va, Level, TargetCr3);

    if (!Pml)
    {
        return FALSE;
    }

    //
    // Change the supervisor bit
    //
    if (Set)
    {
        Pml->Fields.Supervisor = 1;
    }
    else
    {
        Pml->Fields.Supervisor = 0;
    }

    return TRUE;
}
