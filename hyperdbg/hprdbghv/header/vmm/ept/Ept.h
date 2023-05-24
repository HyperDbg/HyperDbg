/**
 * @file Ept.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Contains the headers relating to EPT structures, MTRR and all basic Hooking structures
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					Constants					//
//////////////////////////////////////////////////

/**
 * @brief Page attributes for internal use
 *
 */
#define PAGE_ATTRIB_READ  0x2
#define PAGE_ATTRIB_WRITE 0x4
#define PAGE_ATTRIB_EXEC  0x8

/**
 * @brief The number of 512GB PML4 entries in the page table
 *
 */
#define VMM_EPT_PML4E_COUNT 512

/**
 * @brief The number of 1GB PDPT entries in the page table per 512GB PML4 entry
 *
 */
#define VMM_EPT_PML3E_COUNT 512

/**
 * @brief Then number of 2MB Page Directory entries in the page table per 1GB
 *  PML3 entry
 *
 */
#define VMM_EPT_PML2E_COUNT 512

/**
 * @brief Then number of 4096 byte Page Table entries in the page table per 2MB PML2
 * entry when dynamically split
 *
 */
#define VMM_EPT_PML1E_COUNT 512

/**
 * @brief Integer 2MB
 *
 */
#define SIZE_2_MB ((SIZE_T)(512 * PAGE_SIZE))

/**
 * @brief Offset into the 1st paging structure (4096 byte)
 *
 */
#define ADDRMASK_EPT_PML1_OFFSET(_VAR_) (_VAR_ & 0xFFFULL)

/**
 * @brief Index of the 1st paging structure (4096 byte)
 *
 */
#define ADDRMASK_EPT_PML1_INDEX(_VAR_) ((_VAR_ & 0x1FF000ULL) >> 12)

/**
 * @brief Index of the 2nd paging structure (2MB)
 *
 */
#define ADDRMASK_EPT_PML2_INDEX(_VAR_) ((_VAR_ & 0x3FE00000ULL) >> 21)

/**
 * @brief Index of the 3rd paging structure (1GB)
 *
 */
#define ADDRMASK_EPT_PML3_INDEX(_VAR_) ((_VAR_ & 0x7FC0000000ULL) >> 30)

/**
 * @brief Index of the 4th paging structure (512GB)
 *
 */
#define ADDRMASK_EPT_PML4_INDEX(_VAR_) ((_VAR_ & 0xFF8000000000ULL) >> 39)

//////////////////////////////////////////////////
//	    			Variables 	 	            //
//////////////////////////////////////////////////

/**
 * @brief Vmx-root lock for changing EPT PML1 Entry and Invalidating TLB
 *
 */
volatile LONG Pml1ModificationAndInvalidationLock;

//////////////////////////////////////////////////
//			     Structs Cont.                	//
//////////////////////////////////////////////////

/**
 * @brief Structure for saving EPT Table
 *
 */
typedef struct _VMM_EPT_PAGE_TABLE
{
    /**
     * @brief 28.2.2 Describes 512 contiguous 512GB memory regions each with 512 1GB regions.
     */
    DECLSPEC_ALIGN(PAGE_SIZE)
    EPT_PML4_POINTER PML4[VMM_EPT_PML4E_COUNT];

    /**
     * @brief Describes exactly 512 contiguous 1GB memory regions within a our singular 512GB PML4 region.
     */
    DECLSPEC_ALIGN(PAGE_SIZE)
    EPT_PML3_POINTER PML3[VMM_EPT_PML3E_COUNT];

    /**
     * @brief For each 1GB PML3 entry, create 512 2MB entries to map identity.
     * NOTE: We are using 2MB pages as the smallest paging size in our map, so we do not manage individiual 4096 byte pages.
     * Therefore, we do not allocate any PML1 (4096 byte) paging structures.
     */
    DECLSPEC_ALIGN(PAGE_SIZE)
    EPT_PML2_ENTRY PML2[VMM_EPT_PML3E_COUNT][VMM_EPT_PML2E_COUNT];

} VMM_EPT_PAGE_TABLE, *PVMM_EPT_PAGE_TABLE;

/**
 * @brief MTRR Range Descriptor
 *
 */
typedef struct _MTRR_RANGE_DESCRIPTOR
{
    SIZE_T PhysicalBaseAddress;
    SIZE_T PhysicalEndAddress;
    UCHAR  MemoryType;
} MTRR_RANGE_DESCRIPTOR, *PMTRR_RANGE_DESCRIPTOR;

/**
 * @brief Main structure for saving the state of EPT among the project
 *
 */
#define EPT_MTRR_RANGE_DESCRIPTOR_MAX 0x9
typedef struct _EPT_STATE
{
    LIST_ENTRY            HookedPagesList;                             // A list of the details about hooked pages
    MTRR_RANGE_DESCRIPTOR MemoryRanges[EPT_MTRR_RANGE_DESCRIPTOR_MAX]; // Physical memory ranges described by the BIOS in the MTRRs. Used to build the EPT identity mapping.
    ULONG                 NumberOfEnabledMemoryRanges;                 // Number of memory ranges specified in MemoryRanges
    PVMM_EPT_PAGE_TABLE   EptPageTable;                                // Page table entries for EPT operation
    PVMM_EPT_PAGE_TABLE   ModeBasedEptPageTable;                       // Page table entries for hooks based on mode-based execution control bits
    PVMM_EPT_PAGE_TABLE   ExecuteOnlyEptPageTable;                     // Page table entries for execute-only control bits
    EPT_POINTER           EptPointer;                                  // Extended-Page-Table Pointer
    EPT_POINTER           ModeBasedEptPointer;                         // Extended-Page-Table Pointer for Mode-based execution
    EPT_POINTER           ExecuteOnlyEptPointer;                       // Extended-Page-Table Pointer for execute-only execution

    PVMM_EPT_PAGE_TABLE SecondaryEptPageTable;                         // Secondary Page table entries for EPT operation (Used in debugger mechanisms)

} EPT_STATE, *PEPT_STATE;

/**
 * @brief Split 2MB granularity to 4 KB granularity
 *
 */
typedef struct _VMM_EPT_DYNAMIC_SPLIT
{
    /**
     * @brief The 4096 byte page table entries that correspond to the split 2MB table entry
     *
     */
    DECLSPEC_ALIGN(PAGE_SIZE)
    EPT_PML1_ENTRY PML1[VMM_EPT_PML1E_COUNT];

    /**
     * @brief The pointer to the 2MB entry in the page table which this split is servicing.
     *
     */
    union
    {
        PEPT_PML2_ENTRY   Entry;
        PEPT_PML2_POINTER Pointer;
    };

    /**
     * @brief Linked list entries for each dynamic split
     *
     */
    LIST_ENTRY DynamicSplitList;

} VMM_EPT_DYNAMIC_SPLIT, *PVMM_EPT_DYNAMIC_SPLIT;

//////////////////////////////////////////////////
//				    Functions					//
//////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// Private Interfaces
//

static PEPT_PML2_ENTRY
EptGetPml2Entry(PVMM_EPT_PAGE_TABLE EptPageTable, SIZE_T PhysicalAddress);

static VOID
EptSetupPML2Entry(PEPT_PML2_ENTRY NewEntry, SIZE_T PageFrameNumber);

static BOOLEAN
EptHandlePageHookExit(_Inout_ VIRTUAL_MACHINE_STATE *           VCpu,
                      _In_ VMX_EXIT_QUALIFICATION_EPT_VIOLATION ViolationQualification,
                      _In_ UINT64                               GuestPhysicalAddr);

// ----------------------------------------------------------------------------
// Public Interfaces
//

/**
 * @brief Check for EPT Features
 *
 * @return BOOLEAN
 */
BOOLEAN
EptCheckFeatures();

/**
 * @brief Build MTRR Map
 *
 * @return BOOLEAN
 */
BOOLEAN
EptBuildMtrrMap();

/**
 * @brief Allocates page maps and create identity page table
 *
 * @return PVMM_EPT_PAGE_TABLE identity map page-table
 */
PVMM_EPT_PAGE_TABLE
EptAllocateAndCreateIdentityPageTable();

/**
 * @brief Convert 2MB pages to 4KB pages
 *
 * @param EptPageTable
 * @param PreAllocatedBuffer
 * @param PhysicalAddress
 * @return BOOLEAN
 */
BOOLEAN
EptSplitLargePage(PVMM_EPT_PAGE_TABLE EptPageTable,
                  PVOID               PreAllocatedBuffer,
                  SIZE_T              PhysicalAddress);

/**
 * @brief Initialize EPT Table based on Processor Index
 *
 * @return BOOLEAN
 */
BOOLEAN
EptLogicalProcessorInitialize();

/**
 * @brief Handle EPT Violation
 *
 * @param VCpu The virtual processor's state
 *
 * @return BOOLEAN
 */
BOOLEAN
EptHandleEptViolation(VIRTUAL_MACHINE_STATE * VCpu);

/**
 * @brief Get the PML1 Entry of a special address
 *
 * @param EptPageTable
 * @param PhysicalAddress
 * @return PEPT_PML1_ENTRY
 */
PEPT_PML1_ENTRY
EptGetPml1Entry(PVMM_EPT_PAGE_TABLE EptPageTable, SIZE_T PhysicalAddress);

/**
 * @brief Get the PML1 entry for this physical address if the large page
 * is available then large page of Pml2 is returned
 *
 * @param EptPageTable The EPT Page Table
 * @param PhysicalAddress Physical address that we want to get its PML1
 * @param IsLargePage Shows whether it's a large page or not
 *
 * @return PEPT_PML1_ENTRY Return PEPT_PML1_ENTRY or PEPT_PML2_ENTRY
 */
PVOID
EptGetPml1OrPml2Entry(PVMM_EPT_PAGE_TABLE EptPageTable, SIZE_T PhysicalAddress, BOOLEAN * IsLargePage);

/**
 * @brief Handle vm-exits for Monitor Trap Flag to restore previous state
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
EptHandleMonitorTrapFlag(VIRTUAL_MACHINE_STATE * VCpu);

/**
 * @brief Handle Ept Misconfigurations
 *
 * @return VOID
 */
VOID
EptHandleMisconfiguration();

/**
 * @brief This function set the specific PML1 entry in a spinlock protected area then
 * invalidate the TLB , this function should be called from vmx root-mode
 *
 * @param EntryAddress
 * @param EntryValue
 * @param InvalidationType
 * @return VOID
 */
VOID
EptSetPML1AndInvalidateTLB(_Out_ PEPT_PML1_ENTRY                EntryAddress,
                           _In_ EPT_PML1_ENTRY                  EntryValue,
                           _In_ _Strict_type_match_ INVEPT_TYPE InvalidationType);

/**
 * @brief Check if the breakpoint vm-exit relates to EPT hook or not
 *
 * @param VCpu The virtual processor's state
 *
 * @return BOOLEAN
 */
BOOLEAN
EptCheckAndHandleBreakpoint(VIRTUAL_MACHINE_STATE * VCpu);
