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
//				Debugger Config                 //
//////////////////////////////////////////////////

#define MaximumHiddenBreakpointsOnPage 40

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
//				Unions & Structs    			//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//				      typedefs         			 //
//////////////////////////////////////////////////

typedef EPT_PML4E   EPT_PML4_POINTER, *PEPT_PML4_POINTER;
typedef EPT_PDPTE   EPT_PML3_POINTER, *PEPT_PML3_POINTER;
typedef EPT_PDE_2MB EPT_PML2_ENTRY, *PEPT_PML2_ENTRY;
typedef EPT_PDE     EPT_PML2_POINTER, *PEPT_PML2_POINTER;
typedef EPT_PTE     EPT_PML1_ENTRY, *PEPT_PML1_ENTRY;

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
    EPT_POINTER           EptPointer;                                  // Extended-Page-Table Pointer
    PVMM_EPT_PAGE_TABLE   EptPageTable;                                // Page table entries for EPT operation

    PVMM_EPT_PAGE_TABLE SecondaryEptPageTable; // Secondary Page table entries for EPT operation (Used in debugger mechanisms)

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

/**
 * @brief Structure to save the state of each hooked pages
 * 
 */
typedef struct _EPT_HOOKED_PAGE_DETAIL
{
    DECLSPEC_ALIGN(PAGE_SIZE)
    CHAR FakePageContents[PAGE_SIZE];

    /**
     * @brief Linked list entires for each page hook.
     */
    LIST_ENTRY PageHookList;

    /**
    * @brief The virtual address from the caller prespective view (cr3)
    */
    UINT64 VirtualAddress;

    /**
    * @brief The virtual address of it's enty on g_EptHook2sDetourListHead 
    * this way we can de-allocate the list whenever the hook is finished
    */
    UINT64 AddressOfEptHook2sDetourListEntry;

    /**
     * @brief The base address of the page. Used to find this structure in the list of page hooks
     * when a hook is hit.
     */
    SIZE_T PhysicalBaseAddress;

    /**
    * @brief The base address of the page with fake contents. Used to swap page with fake contents
    * when a hook is hit.
    */
    SIZE_T PhysicalBaseAddressOfFakePageContents;

    /*
     * @brief The page entry in the page tables that this page is targetting.
     */
    PEPT_PML1_ENTRY EntryAddress;

    /**
     * @brief The original page entry. Will be copied back when the hook is removed
     * from the page.
     */
    EPT_PML1_ENTRY OriginalEntry;

    /**
     * @brief The original page entry. Will be copied back when the hook is remove from the page.
     */
    EPT_PML1_ENTRY ChangedEntry;

    /**
    * @brief The buffer of the trampoline function which is used in the inline hook.
    */
    PCHAR Trampoline;

    /**
     * @brief This field shows whether the hook contains a hidden hook for execution or not
     */
    BOOLEAN IsExecutionHook;

    /**
     * @brief If TRUE shows that this is the information about 
     * a hidden breakpoint command (not a monitor or hidden detours)
     */
    BOOLEAN IsHiddenBreakpoint;

    /**
     * @brief Address of hooked pages (multiple breakpoints on a single page)
     * this is only used in hidden breakpoints (not hidden detours)
     */
    UINT64 BreakpointAddresses[MaximumHiddenBreakpointsOnPage];

    /**
     * @brief Character that was previously used in BreakpointAddresses
     * this is only used in hidden breakpoints (not hidden detours)
     */
    CHAR PreviousBytesOnBreakpointAddresses[MaximumHiddenBreakpointsOnPage];

    /**
     * @brief Count of breakpoints (multiple breakpoints on a single page)
     * this is only used in hidden breakpoints (not hidden detours)
     */
    UINT64 CountOfBreakpoints;

} EPT_HOOKED_PAGE_DETAIL, *PEPT_HOOKED_PAGE_DETAIL;

//////////////////////////////////////////////////
//                    Enums		    			//
//////////////////////////////////////////////////

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

static PVMM_EPT_PAGE_TABLE
EptAllocateAndCreateIdentityPageTable();

static BOOLEAN
EptHandlePageHookExit(_Inout_ PGUEST_REGS                       Regs,
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
 * @brief Convert 2MB pages to 4KB pages
 * 
 * @param EptPageTable 
 * @param PreAllocatedBuffer 
 * @param PhysicalAddress 
 * @param CoreIndex 
 * @return BOOLEAN 
 */
BOOLEAN
EptSplitLargePage(PVMM_EPT_PAGE_TABLE EptPageTable,
                  PVOID               PreAllocatedBuffer,
                  SIZE_T              PhysicalAddress,
                  ULONG               CoreIndex);

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
 * @param Regs 
 * @param ExitQualification 
 * @param GuestPhysicalAddr 
 * @return BOOLEAN 
 */
BOOLEAN
EptHandleEptViolation(_Inout_ PGUEST_REGS Regs,
                      _In_ ULONG          ExitQualification,
                      _In_ UINT64         GuestPhysicalAddr);

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
 * @brief Handle vm-exits for Monitor Trap Flag to restore previous state
 * 
 * @param HookedEntry 
 * @return VOID 
 */
VOID
EptHandleMonitorTrapFlag(PEPT_HOOKED_PAGE_DETAIL HookedEntry);

/**
 * @brief Handle Ept Misconfigurations
 * 
 * @param GuestAddress 
 * @return VOID 
 */
VOID
EptHandleMisconfiguration(UINT64 GuestAddress);

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
