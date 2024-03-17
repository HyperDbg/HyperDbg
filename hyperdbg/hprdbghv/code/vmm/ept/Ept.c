/**
 * @file Ept.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @author Gbps
 * @author Matthijs Lavrijsen (mattiwatti@gmail.com)
 * @brief The implementation of functions relating to the Extended Page Table (a.k.a. EPT)
 * @details Some of the codes are re-used from Gbps/gbhv (https://github.com/Gbps/gbhv)
 * @version 0.1
 * @date 2020-04-10
 *
 * @copyright This project is released under the GNU Public License v3. The re-used codes from Gbps/gbhv are under CC 4.0 AL terms.
 *
 */
#include "pch.h"

/**
 * @brief Check whether EPT features are present or not
 *
 * @return BOOLEAN Shows whether EPT is supported in this machine or not
 */
BOOLEAN
EptCheckFeatures(VOID)
{
    IA32_VMX_EPT_VPID_CAP_REGISTER VpidRegister;
    IA32_MTRR_DEF_TYPE_REGISTER    MTRRDefType;

    VpidRegister.AsUInt = __readmsr(IA32_VMX_EPT_VPID_CAP);
    MTRRDefType.AsUInt  = __readmsr(IA32_MTRR_DEF_TYPE);

    if (!VpidRegister.PageWalkLength4 || !VpidRegister.MemoryTypeWriteBack || !VpidRegister.Pde2MbPages)
    {
        return FALSE;
    }

    if (!VpidRegister.AdvancedVmexitEptViolationsInformation)
    {
        LogDebugInfo("The processor doesn't report advanced VM-exit information for EPT violations");
    }

    if (!VpidRegister.ExecuteOnlyPages)
    {
        g_CompatibilityCheck.ExecuteOnlySupport = FALSE;
        LogDebugInfo("The processor doesn't support execute-only pages, execute hooks won't work as they're on this feature in our design");
    }
    else
    {
        g_CompatibilityCheck.ExecuteOnlySupport = TRUE;
    }

    if (!MTRRDefType.MtrrEnable)
    {
        LogError("Err, MTRR dynamic ranges are not supported");
        return FALSE;
    }

    LogDebugInfo("All EPT features are present");

    return TRUE;
}

/**
 * @brief Check whether EPT features are present or not
 *
 * @param PageFrameNumber
 * @param IsLargePage
 * @return UINT8 Return desired type of memory for particular small/large page
 */
UINT8
EptGetMemoryType(SIZE_T PageFrameNumber, BOOLEAN IsLargePage)
{
    UINT8                   TargetMemoryType;
    SIZE_T                  AddressOfPage;
    SIZE_T                  CurrentMtrrRange;
    MTRR_RANGE_DESCRIPTOR * CurrentMemoryRange;

    AddressOfPage = IsLargePage ? PageFrameNumber * SIZE_2_MB : PageFrameNumber * PAGE_SIZE;

    TargetMemoryType = (UINT8)-1;

    //
    // For each MTRR range
    //
    for (CurrentMtrrRange = 0; CurrentMtrrRange < g_EptState->NumberOfEnabledMemoryRanges; CurrentMtrrRange++)
    {
        CurrentMemoryRange = &g_EptState->MemoryRanges[CurrentMtrrRange];

        //
        // If the physical address is described by this MTRR
        //
        if (AddressOfPage >= CurrentMemoryRange->PhysicalBaseAddress &&
            AddressOfPage < CurrentMemoryRange->PhysicalEndAddress)
        {
            // LogInfo("0x%X> Range=%llX -> %llX | Begin=%llX End=%llX", PageFrameNumber, AddressOfPage, AddressOfPage + SIZE_2_MB - 1, g_EptState->MemoryRanges[CurrentMtrrRange].PhysicalBaseAddress, g_EptState->MemoryRanges[CurrentMtrrRange].PhysicalEndAddress);

            //
            // 12.11.4.1 MTRR Precedences
            //
            if (CurrentMemoryRange->FixedRange)
            {
                //
                // When the fixed-range MTRRs are enabled, they take priority over the variable-range
                // MTRRs when overlaps in ranges occur.
                //
                TargetMemoryType = CurrentMemoryRange->MemoryType;
                break;
            }

            if (TargetMemoryType == MEMORY_TYPE_UNCACHEABLE)
            {
                //
                // If this is going to be marked uncacheable, then we stop the search as UC always
                // takes precedence
                //
                TargetMemoryType = CurrentMemoryRange->MemoryType;
                break;
            }

            if (TargetMemoryType == MEMORY_TYPE_WRITE_THROUGH || CurrentMemoryRange->MemoryType == MEMORY_TYPE_WRITE_THROUGH)
            {
                if (TargetMemoryType == MEMORY_TYPE_WRITE_BACK)
                {
                    //
                    // If two or more MTRRs overlap and describe the same region, and at least one is WT and
                    // the other one(s) is/are WB, use WT. However, continue looking, as other MTRRs
                    // may still specify the address as UC, which always takes precedence
                    //
                    TargetMemoryType = MEMORY_TYPE_WRITE_THROUGH;
                    continue;
                }
            }

            //
            // Otherwise, just use the last MTRR that describes this address
            //
            TargetMemoryType = CurrentMemoryRange->MemoryType;
        }
    }

    //
    // If no MTRR was found, return the default memory type
    //
    if (TargetMemoryType == (UINT8)-1)
    {
        TargetMemoryType = g_EptState->DefaultMemoryType;
    }

    return TargetMemoryType;
}

/**
 * @brief Build MTRR Map of current physical addresses
 *
 * @return BOOLEAN
 */
BOOLEAN
EptBuildMtrrMap(VOID)
{
    IA32_MTRR_CAPABILITIES_REGISTER MTRRCap;
    IA32_MTRR_PHYSBASE_REGISTER     CurrentPhysBase;
    IA32_MTRR_PHYSMASK_REGISTER     CurrentPhysMask;
    IA32_MTRR_DEF_TYPE_REGISTER     MTRRDefType;
    PMTRR_RANGE_DESCRIPTOR          Descriptor;
    UINT32                          CurrentRegister;
    UINT32                          NumberOfBitsInMask;

    MTRRCap.AsUInt     = __readmsr(IA32_MTRR_CAPABILITIES);
    MTRRDefType.AsUInt = __readmsr(IA32_MTRR_DEF_TYPE);

    //
    // All MTRRs are disabled when clear, and the
    // UC memory type is applied to all of physical memory.
    //
    if (!MTRRDefType.MtrrEnable)
    {
        g_EptState->DefaultMemoryType = MEMORY_TYPE_UNCACHEABLE;
        return TRUE;
    }

    //
    // The IA32_MTRR_DEF_TYPE MSR (named MTRRdefType MSR for the P6 family processors) sets the default
    // properties of the regions of physical memory that are not encompassed by MTRRs
    //
    g_EptState->DefaultMemoryType = (UINT8)MTRRDefType.DefaultMemoryType;

    //
    // The fixed memory ranges are mapped with 11 fixed-range registers of 64 bits each. Each of these registers is
    // divided into 8-bit fields that are used to specify the memory type for each of the sub-ranges the register controls:
    //  - Register IA32_MTRR_FIX64K_00000 - Maps the 512-KByte address range from 0H to 7FFFFH. This range
    //  is divided into eight 64-KByte sub-ranges.
    //
    //  - Registers IA32_MTRR_FIX16K_80000 and IA32_MTRR_FIX16K_A0000 - Maps the two 128-KByte
    //  address ranges from 80000H to BFFFFH. This range is divided into sixteen 16-KByte sub-ranges, 8 ranges per
    //  register.
    //
    //  - Registers IA32_MTRR_FIX4K_C0000 through IA32_MTRR_FIX4K_F8000 - Maps eight 32-KByte
    //  address ranges from C0000H to FFFFFH. This range is divided into sixty-four 4-KByte sub-ranges, 8 ranges per
    //  register.
    //
    if (MTRRCap.FixedRangeSupported && MTRRDefType.FixedRangeMtrrEnable)
    {
        const UINT32               K64Base  = 0x0;
        const UINT32               K64Size  = 0x10000;
        IA32_MTRR_FIXED_RANGE_TYPE K64Types = {__readmsr(IA32_MTRR_FIX64K_00000)};
        for (unsigned int i = 0; i < 8; i++)
        {
            Descriptor                      = &g_EptState->MemoryRanges[g_EptState->NumberOfEnabledMemoryRanges++];
            Descriptor->MemoryType          = K64Types.s.Types[i];
            Descriptor->PhysicalBaseAddress = K64Base + (K64Size * i);
            Descriptor->PhysicalEndAddress  = K64Base + (K64Size * i) + (K64Size - 1);
            Descriptor->FixedRange          = TRUE;
        }

        const UINT32 K16Base = 0x80000;
        const UINT32 K16Size = 0x4000;
        for (unsigned int i = 0; i < 2; i++)
        {
            IA32_MTRR_FIXED_RANGE_TYPE K16Types = {__readmsr(IA32_MTRR_FIX16K_80000 + i)};
            for (unsigned int j = 0; j < 8; j++)
            {
                Descriptor                      = &g_EptState->MemoryRanges[g_EptState->NumberOfEnabledMemoryRanges++];
                Descriptor->MemoryType          = K16Types.s.Types[j];
                Descriptor->PhysicalBaseAddress = (K16Base + (i * K16Size * 8)) + (K16Size * j);
                Descriptor->PhysicalEndAddress  = (K16Base + (i * K16Size * 8)) + (K16Size * j) + (K16Size - 1);
                Descriptor->FixedRange          = TRUE;
            }
        }

        const UINT32 K4Base = 0xC0000;
        const UINT32 K4Size = 0x1000;
        for (unsigned int i = 0; i < 8; i++)
        {
            IA32_MTRR_FIXED_RANGE_TYPE K4Types = {__readmsr(IA32_MTRR_FIX4K_C0000 + i)};

            for (unsigned int j = 0; j < 8; j++)
            {
                Descriptor                      = &g_EptState->MemoryRanges[g_EptState->NumberOfEnabledMemoryRanges++];
                Descriptor->MemoryType          = K4Types.s.Types[j];
                Descriptor->PhysicalBaseAddress = (K4Base + (i * K4Size * 8)) + (K4Size * j);
                Descriptor->PhysicalEndAddress  = (K4Base + (i * K4Size * 8)) + (K4Size * j) + (K4Size - 1);
                Descriptor->FixedRange          = TRUE;
            }
        }
    }

    for (CurrentRegister = 0; CurrentRegister < MTRRCap.VariableRangeCount; CurrentRegister++)
    {
        //
        // For each dynamic register pair
        //
        CurrentPhysBase.AsUInt = __readmsr(IA32_MTRR_PHYSBASE0 + (CurrentRegister * 2));
        CurrentPhysMask.AsUInt = __readmsr(IA32_MTRR_PHYSMASK0 + (CurrentRegister * 2));

        //
        // Is the range enabled?
        //
        if (CurrentPhysMask.Valid)
        {
            //
            // We only need to read these once because the ISA dictates that MTRRs are
            // to be synchronized between all processors during BIOS initialization.
            //
            Descriptor = &g_EptState->MemoryRanges[g_EptState->NumberOfEnabledMemoryRanges++];

            //
            // Calculate the base address in bytes
            //
            Descriptor->PhysicalBaseAddress = CurrentPhysBase.PageFrameNumber * PAGE_SIZE;

            //
            // Calculate the total size of the range
            // The lowest bit of the mask that is set to 1 specifies the size of the range
            //
            _BitScanForward64((ULONG *)&NumberOfBitsInMask, CurrentPhysMask.PageFrameNumber * PAGE_SIZE);

            //
            // Size of the range in bytes + Base Address
            //
            Descriptor->PhysicalEndAddress = Descriptor->PhysicalBaseAddress + ((1ULL << NumberOfBitsInMask) - 1ULL);

            //
            // Memory Type (cacheability attributes)
            //
            Descriptor->MemoryType = (UCHAR)CurrentPhysBase.Type;

            Descriptor->FixedRange = FALSE;

            LogDebugInfo("MTRR Range: Base=0x%llx End=0x%llx Type=0x%x", Descriptor->PhysicalBaseAddress, Descriptor->PhysicalEndAddress, Descriptor->MemoryType);
        }
    }

    LogDebugInfo("Total MTRR ranges committed: 0x%x", g_EptState->NumberOfEnabledMemoryRanges);

    return TRUE;
}

/**
 * @brief Get the PML1 entry for this physical address if the page is split
 *
 * @param EptPageTable The EPT Page Table
 * @param PhysicalAddress Physical address that we want to get its PML1
 * @return PEPT_PML1_ENTRY Return NULL if the address is invalid or the page wasn't already split
 */
PEPT_PML1_ENTRY
EptGetPml1Entry(PVMM_EPT_PAGE_TABLE EptPageTable, SIZE_T PhysicalAddress)
{
    SIZE_T            Directory, DirectoryPointer, PML4Entry;
    PEPT_PML2_ENTRY   PML2;
    PEPT_PML1_ENTRY   PML1;
    PEPT_PML2_POINTER PML2Pointer;

    Directory        = ADDRMASK_EPT_PML2_INDEX(PhysicalAddress);
    DirectoryPointer = ADDRMASK_EPT_PML3_INDEX(PhysicalAddress);
    PML4Entry        = ADDRMASK_EPT_PML4_INDEX(PhysicalAddress);

    //
    // Addresses above 512GB are invalid because it is > physical address bus width
    //
    if (PML4Entry > 0)
    {
        return NULL;
    }

    PML2 = &EptPageTable->PML2[DirectoryPointer][Directory];

    //
    // Check to ensure the page is split
    //
    if (PML2->LargePage)
    {
        return NULL;
    }

    //
    // Conversion to get the right PageFrameNumber.
    // These pointers occupy the same place in the table and are directly convertible.
    //
    PML2Pointer = (PEPT_PML2_POINTER)PML2;

    //
    // If it is, translate to the PML1 pointer
    //
    PML1 = (PEPT_PML1_ENTRY)PhysicalAddressToVirtualAddress(PML2Pointer->PageFrameNumber * PAGE_SIZE);

    if (!PML1)
    {
        return NULL;
    }

    //
    // Index into PML1 for that address
    //
    PML1 = &PML1[ADDRMASK_EPT_PML1_INDEX(PhysicalAddress)];

    return PML1;
}

/**
 * @brief Get the PML1 entry for this physical address if the large page
 * is available then large page of Pml2 is returned
 *
 * @param EptPageTable The EPT Page Table
 * @param PhysicalAddress Physical address that we want to get its PML1
 * @param IsLargePage Shows whether it's a large page or not
 *
 * @return PVOID Return PEPT_PML1_ENTRY or PEPT_PML2_ENTRY
 */
PVOID
EptGetPml1OrPml2Entry(PVMM_EPT_PAGE_TABLE EptPageTable, SIZE_T PhysicalAddress, BOOLEAN * IsLargePage)
{
    SIZE_T            Directory, DirectoryPointer, PML4Entry;
    PEPT_PML2_ENTRY   PML2;
    PEPT_PML1_ENTRY   PML1;
    PEPT_PML2_POINTER PML2Pointer;

    Directory        = ADDRMASK_EPT_PML2_INDEX(PhysicalAddress);
    DirectoryPointer = ADDRMASK_EPT_PML3_INDEX(PhysicalAddress);
    PML4Entry        = ADDRMASK_EPT_PML4_INDEX(PhysicalAddress);

    //
    // Addresses above 512GB are invalid because it is > physical address bus width
    //
    if (PML4Entry > 0)
    {
        return NULL;
    }

    PML2 = &EptPageTable->PML2[DirectoryPointer][Directory];

    //
    // Check to ensure the page is split
    //
    if (PML2->LargePage)
    {
        *IsLargePage = TRUE;
        return PML2;
    }

    //
    // Conversion to get the right PageFrameNumber.
    // These pointers occupy the same place in the table and are directly convertible.
    //
    PML2Pointer = (PEPT_PML2_POINTER)PML2;

    //
    // If it is, translate to the PML1 pointer
    //
    PML1 = (PEPT_PML1_ENTRY)PhysicalAddressToVirtualAddress(PML2Pointer->PageFrameNumber * PAGE_SIZE);

    if (!PML1)
    {
        return NULL;
    }

    //
    // Index into PML1 for that address
    //
    PML1 = &PML1[ADDRMASK_EPT_PML1_INDEX(PhysicalAddress)];

    *IsLargePage = FALSE;
    return PML1;
}

/**
 * @brief Get the PML2 entry for this physical address
 *
 * @param EptPageTable The EPT Page Table
 * @param PhysicalAddress Physical Address that we want to get its PML2
 * @return PEPT_PML2_ENTRY The PML2 Entry Structure
 */
PEPT_PML2_ENTRY
EptGetPml2Entry(PVMM_EPT_PAGE_TABLE EptPageTable, SIZE_T PhysicalAddress)
{
    SIZE_T          Directory, DirectoryPointer, PML4Entry;
    PEPT_PML2_ENTRY PML2;

    Directory        = ADDRMASK_EPT_PML2_INDEX(PhysicalAddress);
    DirectoryPointer = ADDRMASK_EPT_PML3_INDEX(PhysicalAddress);
    PML4Entry        = ADDRMASK_EPT_PML4_INDEX(PhysicalAddress);

    //
    // Addresses above 512GB are invalid because it is > physical address bus width
    //
    if (PML4Entry > 0)
    {
        return NULL;
    }

    PML2 = &EptPageTable->PML2[DirectoryPointer][Directory];
    return PML2;
}

/**
 * @brief Split 2MB (LargePage) into 4kb pages
 *
 * @param EptPageTable The EPT Page Table
 * @param PreAllocatedBuffer The address of pre-allocated buffer
 * @param PhysicalAddress Physical address of where we want to split
 *
 * @return BOOLEAN Returns true if it was successful or false if there was an error
 */
BOOLEAN
EptSplitLargePage(PVMM_EPT_PAGE_TABLE EptPageTable,
                  PVOID               PreAllocatedBuffer,
                  SIZE_T              PhysicalAddress)
{
    PVMM_EPT_DYNAMIC_SPLIT NewSplit;
    EPT_PML1_ENTRY         EntryTemplate;
    SIZE_T                 EntryIndex;
    PEPT_PML2_ENTRY        TargetEntry;
    EPT_PML2_POINTER       NewPointer;

    //
    // Find the PML2 entry that's currently used
    //
    TargetEntry = EptGetPml2Entry(EptPageTable, PhysicalAddress);

    if (!TargetEntry)
    {
        LogError("Err, an invalid physical address passed");
        return FALSE;
    }

    //
    // If this large page is not marked a large page, that means it's a pointer already.
    // That page is therefore already split.
    //
    if (!TargetEntry->LargePage)
    {
        //
        // As it's a large page and we request a pool for it, we need to
        // free the pool because it's not used anymore
        //
        PoolManagerFreePool((UINT64)PreAllocatedBuffer);

        return TRUE;
    }

    //
    // Allocate the PML1 entries
    //
    NewSplit = (PVMM_EPT_DYNAMIC_SPLIT)PreAllocatedBuffer;
    if (!NewSplit)
    {
        LogError("Err, failed to allocate dynamic split memory");
        return FALSE;
    }
    RtlZeroMemory(NewSplit, sizeof(VMM_EPT_DYNAMIC_SPLIT));

    //
    // Point back to the entry in the dynamic split for easy reference for which entry that
    // dynamic split is for
    //
    NewSplit->u.Entry = TargetEntry;

    //
    // Make a template for RWX
    //
    EntryTemplate.AsUInt        = 0;
    EntryTemplate.ReadAccess    = 1;
    EntryTemplate.WriteAccess   = 1;
    EntryTemplate.ExecuteAccess = 1;

    //
    // copy other bits from target entry
    //
    EntryTemplate.MemoryType = TargetEntry->MemoryType;
    EntryTemplate.IgnorePat  = TargetEntry->IgnorePat;
    EntryTemplate.SuppressVe = TargetEntry->SuppressVe;

    //
    // Copy the template into all the PML1 entries
    //
    __stosq((SIZE_T *)&NewSplit->PML1[0], EntryTemplate.AsUInt, VMM_EPT_PML1E_COUNT);

    //
    // Set the page frame numbers for identity mapping
    //
    for (EntryIndex = 0; EntryIndex < VMM_EPT_PML1E_COUNT; EntryIndex++)
    {
        //
        // Convert the 2MB page frame number to the 4096 page entry number plus the offset into the frame
        //
        NewSplit->PML1[EntryIndex].PageFrameNumber = ((TargetEntry->PageFrameNumber * SIZE_2_MB) / PAGE_SIZE) + EntryIndex;
        NewSplit->PML1[EntryIndex].MemoryType      = EptGetMemoryType(NewSplit->PML1[EntryIndex].PageFrameNumber, FALSE);
    }

    //
    // Allocate a new pointer which will replace the 2MB entry with a pointer to 512 4096 byte entries
    //
    NewPointer.AsUInt          = 0;
    NewPointer.WriteAccess     = 1;
    NewPointer.ReadAccess      = 1;
    NewPointer.ExecuteAccess   = 1;
    NewPointer.PageFrameNumber = (SIZE_T)VirtualAddressToPhysicalAddress(&NewSplit->PML1[0]) / PAGE_SIZE;

    //
    // Now, replace the entry in the page table with our new split pointer
    //
    RtlCopyMemory(TargetEntry, &NewPointer, sizeof(NewPointer));

    return TRUE;
}

/**
 * @brief Check if potential large page doesn't land on two or more different cache memory types
 *
 * @param PageFrameNumber PFN (Physical Address)
 * @return BOOLEAN
 */
BOOLEAN
EptIsValidForLargePage(SIZE_T PageFrameNumber)
{
    SIZE_T                  StartAddressOfPage = PageFrameNumber * SIZE_2_MB;
    SIZE_T                  EndAddressOfPage   = StartAddressOfPage + (SIZE_2_MB - 1);
    MTRR_RANGE_DESCRIPTOR * CurrentMemoryRange;
    SIZE_T                  CurrentMtrrRange;

    for (CurrentMtrrRange = 0; CurrentMtrrRange < g_EptState->NumberOfEnabledMemoryRanges; CurrentMtrrRange++)
    {
        CurrentMemoryRange = &g_EptState->MemoryRanges[CurrentMtrrRange];

        if ((StartAddressOfPage <= CurrentMemoryRange->PhysicalEndAddress &&
             EndAddressOfPage > CurrentMemoryRange->PhysicalEndAddress) ||
            (StartAddressOfPage < CurrentMemoryRange->PhysicalBaseAddress &&
             EndAddressOfPage >= CurrentMemoryRange->PhysicalBaseAddress))
        {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * @brief Set up PML2 Entries
 *
 * @param EptPageTable
 * @param NewEntry The PML2 Entry
 * @param PageFrameNumber PFN (Physical Address)
 * @return VOID
 */
BOOLEAN
EptSetupPML2Entry(PVMM_EPT_PAGE_TABLE EptPageTable, PEPT_PML2_ENTRY NewEntry, SIZE_T PageFrameNumber)
{
    PVOID TargetBuffer;

    //
    // Each of the 512 collections of 512 PML2 entries is setup here
    // This will, in total, identity map every physical address from 0x0
    // to physical address 0x8000000000 (512GB of memory)
    // ((EntryGroupIndex * VMM_EPT_PML2E_COUNT) + EntryIndex) * 2MB is
    // the actual physical address we're mapping
    //
    NewEntry->PageFrameNumber = PageFrameNumber;

    if (EptIsValidForLargePage(PageFrameNumber))
    {
        NewEntry->MemoryType = EptGetMemoryType(PageFrameNumber, TRUE);

        return TRUE;
    }
    else
    {
        TargetBuffer = (PVOID)CrsAllocateNonPagedPool(sizeof(VMM_EPT_DYNAMIC_SPLIT));

        if (!TargetBuffer)
        {
            LogError("Err, cannot allocate page for splitting edge large pages");
            return FALSE;
        }

        return EptSplitLargePage(EptPageTable, TargetBuffer, PageFrameNumber * SIZE_2_MB);
    }
}

/**
 * @brief Allocates page maps and create identity page table
 *
 * @return PVMM_EPT_PAGE_TABLE identity map page-table
 */
PVMM_EPT_PAGE_TABLE
EptAllocateAndCreateIdentityPageTable(VOID)
{
    PVMM_EPT_PAGE_TABLE PageTable;
    EPT_PML3_POINTER    RWXTemplate;
    EPT_PML2_ENTRY      PML2EntryTemplate;
    SIZE_T              EntryGroupIndex;
    SIZE_T              EntryIndex;

    //
    // Allocate all paging structures as 4KB aligned pages
    //

    //
    // Allocate address anywhere in the OS's memory space and
    // zero out all entries to ensure all unused entries are marked Not Present
    //
    PageTable = CrsAllocateContiguousZeroedMemory(sizeof(VMM_EPT_PAGE_TABLE));

    if (PageTable == NULL)
    {
        LogError("Err, failed to allocate memory for PageTable");
        return NULL;
    }

    //
    // Mark the first 512GB PML4 entry as present, which allows us to manage up
    // to 512GB of discrete paging structures.
    //
    PageTable->PML4[0].PageFrameNumber = (SIZE_T)VirtualAddressToPhysicalAddress(&PageTable->PML3[0]) / PAGE_SIZE;
    PageTable->PML4[0].ReadAccess      = 1;
    PageTable->PML4[0].WriteAccess     = 1;
    PageTable->PML4[0].ExecuteAccess   = 1;

    //
    // Now mark each 1GB PML3 entry as RWX and map each to their PML2 entry
    //

    //
    // Ensure stack memory is cleared
    //
    RWXTemplate.AsUInt = 0;

    //
    // Set up one 'template' RWX PML3 entry and copy it into each of the 512 PML3 entries
    // Using the same method as SimpleVisor for copying each entry using intrinsics.
    //
    RWXTemplate.ReadAccess    = 1;
    RWXTemplate.WriteAccess   = 1;
    RWXTemplate.ExecuteAccess = 1;

    //
    // Copy the template into each of the 512 PML3 entry slots
    //
    __stosq((SIZE_T *)&PageTable->PML3[0], RWXTemplate.AsUInt, VMM_EPT_PML3E_COUNT);

    //
    // For each of the 512 PML3 entries
    //
    for (EntryIndex = 0; EntryIndex < VMM_EPT_PML3E_COUNT; EntryIndex++)
    {
        //
        // Map the 1GB PML3 entry to 512 PML2 (2MB) entries to describe each large page.
        // NOTE: We do *not* manage any PML1 (4096 byte) entries and do not allocate them.
        //
        PageTable->PML3[EntryIndex].PageFrameNumber = (SIZE_T)VirtualAddressToPhysicalAddress(&PageTable->PML2[EntryIndex][0]) / PAGE_SIZE;
    }

    PML2EntryTemplate.AsUInt = 0;

    //
    // All PML2 entries will be RWX and 'present'
    //
    PML2EntryTemplate.WriteAccess   = 1;
    PML2EntryTemplate.ReadAccess    = 1;
    PML2EntryTemplate.ExecuteAccess = 1;

    //
    // We are using 2MB large pages, so we must mark this 1 here
    //
    PML2EntryTemplate.LargePage = 1;

    //
    // For each collection of 512 PML2 entries (512 collections * 512 entries per collection),
    // mark it RWX using the same template above.
    // This marks the entries as "Present" regardless of if the actual system has memory at
    // this region or not. We will cause a fault in our EPT handler if the guest access a page
    // outside a usable range, despite the EPT frame being present here.
    //
    __stosq((SIZE_T *)&PageTable->PML2[0], PML2EntryTemplate.AsUInt, VMM_EPT_PML3E_COUNT * VMM_EPT_PML2E_COUNT);

    //
    // For each of the 512 collections of 512 2MB PML2 entries
    //
    for (EntryGroupIndex = 0; EntryGroupIndex < VMM_EPT_PML3E_COUNT; EntryGroupIndex++)
    {
        //
        // For each 2MB PML2 entry in the collection
        //
        for (EntryIndex = 0; EntryIndex < VMM_EPT_PML2E_COUNT; EntryIndex++)
        {
            //
            // Setup the memory type and frame number of the PML2 entry
            //
            EptSetupPML2Entry(PageTable, &PageTable->PML2[EntryGroupIndex][EntryIndex], (EntryGroupIndex * VMM_EPT_PML2E_COUNT) + EntryIndex);
        }
    }

    return PageTable;
}

/**
 * @brief Initialize EPT for an individual logical processor
 * @details Creates an identity mapped page table and sets up an EPTP to be applied to the VMCS later
 *
 * @return BOOLEAN
 */
BOOLEAN
EptLogicalProcessorInitialize(VOID)
{
    ULONG               ProcessorsCount;
    PVMM_EPT_PAGE_TABLE PageTable;
    EPT_POINTER         EPTP = {0};

    //
    // Get number of processors
    //
    ProcessorsCount = KeQueryActiveProcessorCount(0);

    for (size_t i = 0; i < ProcessorsCount; i++)
    {
        //
        // Allocate the identity mapped page table
        //
        PageTable = EptAllocateAndCreateIdentityPageTable();

        if (!PageTable)
        {
            //
            // Try to deallocate previous pools (if any)
            //
            for (size_t j = 0; j < ProcessorsCount; j++)
            {
                if (g_GuestState[j].EptPageTable != NULL)
                {
                    MmFreeContiguousMemory(g_GuestState[j].EptPageTable);
                    g_GuestState[j].EptPageTable = NULL;
                }
            }

            LogError("Err, unable to allocate memory for EPT");
            return FALSE;
        }

        //
        // Virtual address to the page table to keep track of it for later freeing
        //
        g_GuestState[i].EptPageTable = PageTable;

        //
        // Use default memory type
        //
        EPTP.MemoryType = g_EptState->DefaultMemoryType;

        //
        // We might utilize the 'access' and 'dirty' flag features in the dirty logging mechanism
        //
        EPTP.EnableAccessAndDirtyFlags = TRUE;

        //
        // Bits 5:3 (1 less than the EPT page-walk length) must be 3, indicating an EPT page-walk length of 4;
        // see Section 28.2.2
        //
        EPTP.PageWalkLength = 3;

        //
        // The physical page number of the page table we will be using
        //
        EPTP.PageFrameNumber = (SIZE_T)VirtualAddressToPhysicalAddress(&PageTable->PML4) / PAGE_SIZE;

        //
        // We will write the EPTP to the VMCS later
        //
        g_GuestState[i].EptPointer = EPTP;
    }

    return TRUE;
}

/**
 * @brief Check if this exit is due to a violation caused by a currently hooked page
 * @details If the memory access attempt was RW and the page was marked executable, the page is swapped with
 * the original page.
 *
 * If the memory access attempt was execute and the page was marked not executable, the page is swapped with
 * the hooked page.
 *
 * @param VCpu The virtual processor's state * @param ViolationQualification The violation qualification in vm-exit
 * @param GuestPhysicalAddr The GUEST_PHYSICAL_ADDRESS that caused this EPT violation
 * @return BOOLEAN Returns true if it was successful or false if the violation was not due to a page hook
 */
_Use_decl_annotations_
BOOLEAN
EptHandlePageHookExit(VIRTUAL_MACHINE_STATE *              VCpu,
                      VMX_EXIT_QUALIFICATION_EPT_VIOLATION ViolationQualification,
                      UINT64                               GuestPhysicalAddr)
{
    PVOID   TargetPage;
    UINT64  CurrentRip;
    UINT32  CurrentInstructionLength;
    BOOLEAN IsHandled               = FALSE;
    BOOLEAN ResultOfHandlingHook    = FALSE;
    BOOLEAN IgnoreReadOrWriteOrExec = FALSE;
    BOOLEAN IsExecViolation         = FALSE;

    LIST_FOR_EACH_LINK(g_EptState->HookedPagesList, EPT_HOOKED_PAGE_DETAIL, PageHookList, HookedEntry)
    {
        if (HookedEntry->PhysicalBaseAddress == (SIZE_T)PAGE_ALIGN(GuestPhysicalAddr))
        {
            //
            // *** We found an address that matches the details ***
            //

            //
            // Returning true means that the caller should return to the ept state to
            // the previous state when this instruction is executed
            // by setting the Monitor Trap Flag. Return false means that nothing special
            // for the caller to do
            //

            //
            // Reaching here means that the hooks was actually caused VM-exit because of
            // our configurations, but here we double whether the hook needs to trigger
            // any event or not because the hooking address (physical) might not be in the
            // target range. For example we might hook 0x123b000 to 0x123b300 but the hook
            // happens on 0x123b4600, so we perform the necessary checks here
            //

            if (GuestPhysicalAddr >= HookedEntry->StartOfTargetPhysicalAddress && GuestPhysicalAddr <= HookedEntry->EndOfTargetPhysicalAddress)
            {
                ResultOfHandlingHook = EptHookHandleHookedPage(VCpu,
                                                               HookedEntry,
                                                               ViolationQualification,
                                                               GuestPhysicalAddr,
                                                               &HookedEntry->LastContextState,
                                                               &IgnoreReadOrWriteOrExec,
                                                               &IsExecViolation);
            }
            else
            {
                //
                // Here we assume the hook is handled as the hook needs to be
                // restored (just not within the range)
                //
                ResultOfHandlingHook = TRUE;
            }

            if (ResultOfHandlingHook)
            {
                //
                // Here we check whether the event should be ignored or not,
                // if we don't apply the below restorations routines, the event
                // won't redo and the emulation of the memory access is passed
                //
                if (!IgnoreReadOrWriteOrExec)
                {
                    //
                    // Pointer to the page entry in the page table
                    //
                    TargetPage = EptGetPml1Entry(VCpu->EptPageTable, HookedEntry->PhysicalBaseAddress);

                    //
                    // Restore to its original entry for one instruction
                    //
                    EptSetPML1AndInvalidateTLB(VCpu,
                                               TargetPage,
                                               HookedEntry->OriginalEntry,
                                               InveptSingleContext);

                    //
                    // Next we have to save the current hooked entry to restore on the next instruction's vm-exit
                    //
                    VCpu->MtfEptHookRestorePoint = HookedEntry;

                    //
                    // The following codes are added because we realized if the execution takes long then
                    // the execution might be switched to another routines, thus, MTF might conclude on
                    // another routine and we might (and will) trigger the same instruction soon
                    //

                    //
                    // We have to set Monitor trap flag and give it the HookedEntry to work with
                    //
                    HvEnableMtfAndChangeExternalInterruptState(VCpu);
                }
            }

            //
            // Indicate that we handled the ept violation
            //
            IsHandled = TRUE;

            //
            // Get out of the loop
            //
            break;
        }
    }

    //
    // Check whether the event should be ignored or not
    //
    if (IgnoreReadOrWriteOrExec)
    {
        //
        // Do not redo the instruction (EPT hooks won't affect the VMCS_VMEXIT_INSTRUCTION_LENGTH),
        // thus, we use custom length diassembler engine to ignore the instruction at target address
        //

        // HvPerformRipIncrement(VCpu); // invalid because EPT Violation won't affect VMCS_VMEXIT_INSTRUCTION_LENGTH
        HvSuppressRipIncrement(VCpu); // Just to make sure nothing is added to the address

        //
        // If the target violation is for READ/WRITE, we ignore the current instruction and move to the
        // next instruction, but if the violation is for execute access, then we just won't increment the RIP
        //
        if (!IsExecViolation)
        {
            //
            // Get the RIP here as the RIP might be changed by the user and thus is not valid to be read
            // from the VCpu
            //
            CurrentRip               = HvGetRip();
            CurrentInstructionLength = DisassemblerLengthDisassembleEngineInVmxRootOnTargetProcess((PVOID)CurrentRip, CommonIsGuestOnUsermode32Bit());

            CurrentRip = CurrentRip + CurrentInstructionLength;

            HvSetRip(CurrentRip);
        }
    }
    else
    {
        //
        // Redo the instruction (it's also not necessary as the EPT Violation won't affect VMCS_VMEXIT_INSTRUCTION_LENGTH)
        //
        HvSuppressRipIncrement(VCpu);
    }

    return IsHandled;
}

/**
 * @brief Handle VM exits for EPT violations
 * @details Violations are thrown whenever an operation is performed on an EPT entry
 * that does not provide permissions to access that page
 *
 * @param VCpu The virtual processor's state
 * @return BOOLEAN Return true if the violation was handled by the page hook handler
 * and false if it was not handled
 */
BOOLEAN
EptHandleEptViolation(VIRTUAL_MACHINE_STATE * VCpu)
{
    UINT64                               GuestPhysicalAddr;
    VMX_EXIT_QUALIFICATION_EPT_VIOLATION ViolationQualification = {.AsUInt = VCpu->ExitQualification};

    //
    // Reading guest physical address
    //
    __vmx_vmread(VMCS_GUEST_PHYSICAL_ADDRESS, &GuestPhysicalAddr);

    if (ExecTrapHandleEptViolationVmexit(VCpu, &ViolationQualification))
    {
        return TRUE;
    }
    else if (EptHandlePageHookExit(VCpu, ViolationQualification, GuestPhysicalAddr))
    {
        //
        // Handled by page hook code
        //
        return TRUE;
    }
    else if (VmmCallbackUnhandledEptViolation(VCpu->CoreId, (UINT64)ViolationQualification.AsUInt, GuestPhysicalAddr))
    {
        //
        // Check whether this violation is meaningful for the application or not
        //
        return TRUE;
    }

    LogError("Err, unexpected EPT violation at RIP: %llx", VCpu->LastVmexitRip);
    DbgBreakPoint();
    //
    // Redo the instruction that caused the exception
    //
    return FALSE;
}

/**
 * @brief Handle vm-exits for EPT Misconfiguration
 *
 * @param GuestAddress
 * @return VOID
 */
VOID
EptHandleMisconfiguration(VOID)
{
    UINT64 GuestPhysicalAddr = 0;

    __vmx_vmread(VMCS_GUEST_PHYSICAL_ADDRESS, &GuestPhysicalAddr);

    LogInfo("EPT Misconfiguration!");

    LogError("Err, a field in the EPT paging structure was invalid, faulting guest address : 0x%llx",
             GuestPhysicalAddr);

    //
    // We can't continue now.
    // EPT misconfiguration is a fatal exception that will probably crash the OS if we don't get out now
    //
}

/**
 * @brief This function set the specific PML1 entry in a spinlock protected area then invalidate the TLB
 * @details This function should be called from vmx root-mode
 *
 * @param VCpu The virtual processor's state
 * @param EntryAddress PML1 entry information (the target address)
 * @param EntryValue The value of pm1's entry (the value that should be replaced)
 * @param InvalidationType type of invalidation
 * @return VOID
 */
_Use_decl_annotations_
VOID
EptSetPML1AndInvalidateTLB(VIRTUAL_MACHINE_STATE * VCpu,
                           PEPT_PML1_ENTRY         EntryAddress,
                           EPT_PML1_ENTRY          EntryValue,
                           INVEPT_TYPE             InvalidationType)
{
    //
    // set the value
    //
    EntryAddress->AsUInt = EntryValue.AsUInt;

    //
    // invalidate the cache
    //
    if (InvalidationType == InveptSingleContext)
    {
        EptInveptSingleContext(VCpu->EptPointer.AsUInt);
    }
    else if (InvalidationType == InveptAllContext)
    {
        EptInveptAllContexts();
    }
    else
    {
        LogError("Err, invalid invalidation parameter");
    }
}

/**
 * @brief Perform checking and handling if the breakpoint vm-exit relates to EPT hook or not
 *
 * @param VCpu The virtual processor's state
 * @param GuestRip
 *
 * @return BOOLEAN
 */
BOOLEAN
EptCheckAndHandleEptHookBreakpoints(VIRTUAL_MACHINE_STATE * VCpu, UINT64 GuestRip)
{
    PVOID       TargetPage;
    PLIST_ENTRY TempList;
    BOOLEAN     IsHandledByEptHook = FALSE;

    //
    // ***** Check breakpoint for !epthook *****
    //

    //
    // Check whether the breakpoint was due to a !epthook command or not
    //
    TempList = &g_EptState->HookedPagesList;

    while (&g_EptState->HookedPagesList != TempList->Flink)
    {
        TempList                            = TempList->Flink;
        PEPT_HOOKED_PAGE_DETAIL HookedEntry = CONTAINING_RECORD(TempList, EPT_HOOKED_PAGE_DETAIL, PageHookList);

        if (HookedEntry->IsExecutionHook)
        {
            for (size_t i = 0; i < HookedEntry->CountOfBreakpoints; i++)
            {
                if (HookedEntry->BreakpointAddresses[i] == GuestRip)
                {
                    //
                    // We found an address that matches the details, let's trigger the event
                    //

                    //
                    // As the context to event trigger, we send the rip
                    // of where triggered this event
                    //
                    DispatchEventHiddenHookExecCc(VCpu, (PVOID)GuestRip);

                    //
                    // Pointer to the page entry in the page table
                    //
                    TargetPage = EptGetPml1Entry(VCpu->EptPageTable, HookedEntry->PhysicalBaseAddress);

                    //
                    // Restore to its original entry for one instruction
                    //
                    EptSetPML1AndInvalidateTLB(VCpu,
                                               TargetPage,
                                               HookedEntry->OriginalEntry,
                                               InveptSingleContext);

                    //
                    // Next we have to save the current hooked entry to restore on the next instruction's vm-exit
                    //
                    VCpu->MtfEptHookRestorePoint = HookedEntry;

                    //
                    // The following codes are added because we realized if the execution takes long then
                    // the execution might be switched to another routines, thus, MTF might conclude on
                    // another routine and we might (and will) trigger the same instruction soon
                    //
                    // The following code is not necessary on local debugging (VMI Mode), however, I don't
                    // know why? just things are not reasonable here for me
                    // another weird thing that I observed is the fact if you don't touch the routine related
                    // to the I/O in and out instructions in VMWare then it works perfectly, just touching I/O
                    // for serial is problematic, it might be a VMWare nested-virtualization bug, however, the
                    // below approached proved to be work on both Debug Mode and WMI Mode
                    // If you remove the below codes then when epthook is triggered then the execution stucks
                    // on the same instruction on where the hooks is triggered, so 'p' and 't' commands for
                    // steppings won't work
                    //

                    //
                    // We have to set Monitor trap flag and give it the HookedEntry to work with
                    //
                    HvEnableMtfAndChangeExternalInterruptState(VCpu);

                    //
                    // Indicate that we handled the ept violation
                    //
                    IsHandledByEptHook = TRUE;

                    //
                    // Get out of the loop
                    //
                    break;
                }
            }
        }
    }

    return IsHandledByEptHook;
}

/**
 * @brief Check if the breakpoint vm-exit relates to EPT hook or not
 *
 * @param VCpu The virtual processor's state
 *
 * @return BOOLEAN
 */
BOOLEAN
EptCheckAndHandleBreakpoint(VIRTUAL_MACHINE_STATE * VCpu)
{
    UINT64  GuestRip = 0;
    BOOLEAN IsHandledByEptHook;

    //
    // Reading guest's RIP
    //
    __vmx_vmread(VMCS_GUEST_RIP, &GuestRip);

    //
    // Don't increment rip by default
    //
    HvSuppressRipIncrement(VCpu);

    //
    // Check if it relates to !epthook or not
    //
    IsHandledByEptHook = EptCheckAndHandleEptHookBreakpoints(VCpu, GuestRip);

    return IsHandledByEptHook;
}
