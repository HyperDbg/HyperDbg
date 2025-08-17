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
#define PAGE_ATTRIB_READ             0x2
#define PAGE_ATTRIB_WRITE            0x4
#define PAGE_ATTRIB_EXEC             0x8
#define PAGE_ATTRIB_EXEC_HIDDEN_HOOK 0x10

/**
 * @brief Integer 2MB
 *
 */
#define SIZE_2_MB ((SIZE_T)(512 * PAGE_SIZE))

/**
 * @brief Integer 1GB
 *
 */
#define SIZE_1_GB ((SIZE_T)(512 * SIZE_2_MB))

/**
 * @brief Integer 512GB
 *
 */
#define SIZE_512_GB ((SIZE_T)(512 * SIZE_1_GB))

/**
 * @brief Offset into the 1st paging structure (4096 byte)
 *
 */
#define ADDRMASK_EPT_PML1_OFFSET(_VAR_) ((_VAR_) & 0xFFFULL)

/**
 * @brief Index of the 1st paging structure (4096 byte)
 *
 */
#define ADDRMASK_EPT_PML1_INDEX(_VAR_) (((_VAR_) & 0x1FF000ULL) >> 12)

/**
 * @brief Index of the 2nd paging structure (2MB)
 *
 */
#define ADDRMASK_EPT_PML2_INDEX(_VAR_) (((_VAR_) & 0x3FE00000ULL) >> 21)

/**
 * @brief Index of the 3rd paging structure (1GB)
 *
 */
#define ADDRMASK_EPT_PML3_INDEX(_VAR_) (((_VAR_) & 0x7FC0000000ULL) >> 30)

/**
 * @brief Index of the 4th paging structure (512GB)
 *
 */
#define ADDRMASK_EPT_PML4_INDEX(_VAR_) (((_VAR_) & 0xFF8000000000ULL) >> 39)

//////////////////////////////////////////////////
//			     Structs Cont.                	//
//////////////////////////////////////////////////

/**
 * @brief MTRR Descriptor
 *
 */
typedef struct _MTRR_RANGE_DESCRIPTOR
{
    SIZE_T  PhysicalBaseAddress;
    SIZE_T  PhysicalEndAddress;
    UCHAR   MemoryType;
    BOOLEAN FixedRange;
} MTRR_RANGE_DESCRIPTOR, *PMTRR_RANGE_DESCRIPTOR;

/**
 * @brief Fixed range MTRR
 *
 */
typedef union _IA32_MTRR_FIXED_RANGE_TYPE
{
    UINT64 AsUInt;
    struct
    {
        UINT8 Types[8];
    } s;
} IA32_MTRR_FIXED_RANGE_TYPE;

/**
 * @brief Architecturally defined number of variable range MTRRs
 *
 */
#define MAX_VARIABLE_RANGE_MTRRS 255

/**
 * @brief Architecturally defined number of fixed range MTRRs. 1 register for 64k, 2
 * registers for 16k, 8 registers for 4k, and each register has 8 ranges as per
 * "Fixed Range MTRRs" states.
 *
 */
#define NUM_FIXED_RANGE_MTRRS ((1 + 2 + 8) * RTL_NUMBER_OF_FIELD(IA32_MTRR_FIXED_RANGE_TYPE, s.Types)) // = 88

/**
 * @brief Total number of MTRR descriptors to store
 *
 */
#define NUM_MTRR_ENTRIES (MAX_VARIABLE_RANGE_MTRRS + NUM_FIXED_RANGE_MTRRS) // = 343

/**
 * @brief Main structure for saving the state of EPT among the project
 *
 */
typedef struct _EPT_STATE
{
    LIST_ENTRY            HookedPagesList;                // A list of the details about hooked pages
    MTRR_RANGE_DESCRIPTOR MemoryRanges[NUM_MTRR_ENTRIES]; // Physical memory ranges described by the BIOS in the MTRRs. Used to build the EPT identity mapping.
    UINT32                NumberOfEnabledMemoryRanges;    // Number of memory ranges specified in MemoryRanges
    UINT8                 DefaultMemoryType;
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
    } u;

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

BOOLEAN
EptSetupPML2Entry(PVMM_EPT_PAGE_TABLE EptPageTable, PEPT_PML2_ENTRY NewEntry, SIZE_T PageFrameNumber);

BOOLEAN
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
EptCheckFeatures(VOID);

/**
 * @brief Build MTRR Map
 *
 * @return BOOLEAN
 */
BOOLEAN
EptBuildMtrrMap(VOID);

/**
 * @brief Allocates page maps and create identity page table
 *
 * @return PVMM_EPT_PAGE_TABLE identity map page-table
 */
PVMM_EPT_PAGE_TABLE
EptAllocateAndCreateIdentityPageTable(VOID);

/**
 * @brief Convert large pages to 4KB pages
 *
 * @param EptPageTable
 * @param UsePreAllocatedBuffer
 * @param PhysicalAddress
 * @return BOOLEAN
 */
BOOLEAN
EptSplitLargePage(PVMM_EPT_PAGE_TABLE EptPageTable,
                  BOOLEAN             UsePreAllocatedBuffer,
                  SIZE_T              PhysicalAddress);

/**
 * @brief Split 2MB (LargePage) into 4kb pages
 *
 * @param EptPageTable The EPT Page Table
 * @param PreAllocatedBuffer The address of pre-allocated buffer
 * @param PhysicalAddress Physical address of where we want to split
 *
 * @return BOOLEAN Returns true if it was successful or false if there was an error
 */
PEPT_PML2_ENTRY
EptGetPml2Entry(PVMM_EPT_PAGE_TABLE EptPageTable, SIZE_T PhysicalAddress);

/**
 * @brief Initialize EPT Table based on Processor Index
 *
 * @return BOOLEAN
 */
BOOLEAN
EptLogicalProcessorInitialize(VOID);

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
 * @brief Handle Ept Misconfigurations
 *
 * @return VOID
 */
VOID
    EptHandleMisconfiguration(VOID);

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
EptSetPML1AndInvalidateTLB(_Inout_ VIRTUAL_MACHINE_STATE *      VCpu,
                           _Out_ PEPT_PML1_ENTRY                EntryAddress,
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
