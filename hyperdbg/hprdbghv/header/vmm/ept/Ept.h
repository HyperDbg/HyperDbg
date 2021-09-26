/**
 * @file Ept.h
 * @author Sina Karvandi (sina@rayanfam.com)
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
 * @brief MTRR Physical Base MSRs
 * 
 */
#define MSR_IA32_MTRR_PHYSBASE0 0x00000200
#define MSR_IA32_MTRR_PHYSBASE1 0x00000202
#define MSR_IA32_MTRR_PHYSBASE2 0x00000204
#define MSR_IA32_MTRR_PHYSBASE3 0x00000206
#define MSR_IA32_MTRR_PHYSBASE4 0x00000208
#define MSR_IA32_MTRR_PHYSBASE5 0x0000020A
#define MSR_IA32_MTRR_PHYSBASE6 0x0000020C
#define MSR_IA32_MTRR_PHYSBASE7 0x0000020E
#define MSR_IA32_MTRR_PHYSBASE8 0x00000210
#define MSR_IA32_MTRR_PHYSBASE9 0x00000212

/**
 * @brief MTRR Physical Mask MSRs
 * 
 */
#define MSR_IA32_MTRR_PHYSMASK0 0x00000201
#define MSR_IA32_MTRR_PHYSMASK1 0x00000203
#define MSR_IA32_MTRR_PHYSMASK2 0x00000205
#define MSR_IA32_MTRR_PHYSMASK3 0x00000207
#define MSR_IA32_MTRR_PHYSMASK4 0x00000209
#define MSR_IA32_MTRR_PHYSMASK5 0x0000020B
#define MSR_IA32_MTRR_PHYSMASK6 0x0000020D
#define MSR_IA32_MTRR_PHYSMASK7 0x0000020F
#define MSR_IA32_MTRR_PHYSMASK8 0x00000211
#define MSR_IA32_MTRR_PHYSMASK9 0x00000213

/**
 * @brief Memory Types
 * 
 */
#define MEMORY_TYPE_UNCACHEABLE     0x00000000
#define MEMORY_TYPE_WRITE_COMBINING 0x00000001
#define MEMORY_TYPE_WRITE_THROUGH   0x00000004
#define MEMORY_TYPE_WRITE_PROTECTED 0x00000005
#define MEMORY_TYPE_WRITE_BACK      0x00000006
#define MEMORY_TYPE_INVALID         0x000000FF

/**
 * @brief Page attributes for internal use
 * 
 */
#define PAGE_ATTRIB_READ  0x2
#define PAGE_ATTRIB_WRITE 0x4
#define PAGE_ATTRIB_EXEC  0x8

/**
 * @brief VMX EPT & VPID Capabilities MSR
 * 
 */
#define MSR_IA32_VMX_EPT_VPID_CAP 0x0000048C

/**
 * @brief MTRR Def MSR
 * 
 */
#define MSR_IA32_MTRR_DEF_TYPE 0x000002FF

/**
 * @brief MTRR Capabilities MSR
 * 
 */
#define MSR_IA32_MTRR_CAPABILITIES 0x000000FE

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

/**
 * @details 
 * Linked list for-each macro for traversing LIST_ENTRY structures.
 *
 * _LISTHEAD_ is a pointer to the struct that the list head belongs to.
 * _LISTHEAD_NAME_ is the name of the variable which contains the list head. Should match the same name as the list entry struct member in the actual record.
 * _TARGET_TYPE_ is the type name of the struct of each item in the list
 * _TARGET_NAME_ is the name which will contain the pointer to the item each iteration
 *
 * Example:
 * FOR_EACH_LIST_ENTRY(ProcessorContext->EptPageTable, DynamicSplitList, VMM_EPT_DYNAMIC_SPLIT, Split)
 * 		OsFreeNonpagedMemory(Split);
 * }
 *
 * ProcessorContext->EptPageTable->DynamicSplitList is the head of the list.
 * VMM_EPT_DYNAMIC_SPLIT is the struct of each item in the list.
 * Split is the name of the local variable which will hold the pointer to the item.
 * 
 */
#define FOR_EACH_LIST_ENTRY(_LISTHEAD_, _LISTHEAD_NAME_, _TARGET_TYPE_, _TARGET_NAME_)                                       \
    for (PLIST_ENTRY Entry = _LISTHEAD_->_LISTHEAD_NAME_.Flink; Entry != &_LISTHEAD_->_LISTHEAD_NAME_; Entry = Entry->Flink) \
    {                                                                                                                        \
        P##_TARGET_TYPE_ _TARGET_NAME_ = CONTAINING_RECORD(Entry, _TARGET_TYPE_, _LISTHEAD_NAME_);

/**
 * @details The braces for the block are messy due to the need to define a local variable in the for loop scope.
 * Therefore, this macro just ends the for each block without messing up code editors trying to detect
 * the block indent level.
 */
#define FOR_EACH_LIST_ENTRY_END() }

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

/**
 * @brief Structure of EPT AND VPID CAP
 * 
 */
typedef union _IA32_VMX_EPT_VPID_CAP_REGISTER
{
    struct
    {
        /**
		 * @brief [Bit 0] When set to 1, the processor supports execute-only translations by EPT. This support allows software to
		 * configure EPT paging-structure entries in which bits 1:0 are clear (indicating that data accesses are not allowed) and
		 * bit 2 is set (indicating that instruction fetches are allowed).
		 */
        UINT64 ExecuteOnlyPages : 1;
        UINT64 Reserved1 : 5;

        /**
		 * @brief [Bit 6] Indicates support for a page-walk length of 4.
		 */
        UINT64 PageWalkLength4 : 1;
        UINT64 Reserved2 : 1;

        /**
		 * @brief [Bit 8] When set to 1, the logical processor allows software to configure the EPT paging-structure memory type to be
		 * uncacheable (UC).
		 *
		 * @see Vol3C[24.6.11(Extended-Page-Table Pointer (EPTP))]
		 */
        UINT64 MemoryTypeUncacheable : 1;
        UINT64 Reserved3 : 5;

        /**
		 * @brief [Bit 14] When set to 1, the logical processor allows software to configure the EPT paging-structure memory type to be
		 * write-back (WB).
		 */
        UINT64 MemoryTypeWriteBack : 1;
        UINT64 Reserved4 : 1;

        /**
		 * @brief [Bit 16] When set to 1, the logical processor allows software to configure a EPT PDE to map a 2-Mbyte page (by setting
		 * bit 7 in the EPT PDE).
		 */
        UINT64 Pde2MbPages : 1;

        /**
		 * @brief [Bit 17] When set to 1, the logical processor allows software to configure a EPT PDPTE to map a 1-Gbyte page (by setting
		 * bit 7 in the EPT PDPTE).
		 */
        UINT64 Pdpte1GbPages : 1;
        UINT64 Reserved5 : 2;

        /**
		 * @brief [Bit 20] If bit 20 is read as 1, the INVEPT instruction is supported.
		 *
		 * @see Vol3C[30(VMX INSTRUCTION REFERENCE)]
		 * @see Vol3C[28.3.3.1(Operations that Invalidate Cached Mappings)]
		 */
        UINT64 Invept : 1;

        /**
		 * @brief [Bit 21] When set to 1, accessed and dirty flags for EPT are supported.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */
        UINT64 EptAccessedAndDirtyFlags : 1;

        /**
		 * @brief [Bit 22] When set to 1, the processor reports advanced VM-exit information for EPT violations. This reporting is done
		 * only if this bit is read as 1.
		 *
		 * @see Vol3C[27.2.1(Basic VM-Exit Information)]
		 */
        UINT64 AdvancedVmexitEptViolationsInformation : 1;
        UINT64 Reserved6 : 2;

        /**
		 * @brief [Bit 25] When set to 1, the single-context INVEPT type is supported.
		 *
		 * @see Vol3C[30(VMX INSTRUCTION REFERENCE)]
		 * @see Vol3C[28.3.3.1(Operations that Invalidate Cached Mappings)]
		 */
        UINT64 InveptSingleContext : 1;

        /**
		 * @brief [Bit 26] When set to 1, the all-context INVEPT type is supported.
		 *
		 * @see Vol3C[30(VMX INSTRUCTION REFERENCE)]
		 * @see Vol3C[28.3.3.1(Operations that Invalidate Cached Mappings)]
		 */
        UINT64 InveptAllContexts : 1;
        UINT64 Reserved7 : 5;

        /**
		 * @brief [Bit 32] When set to 1, the INVVPID instruction is supported.
		 */
        UINT64 Invvpid : 1;
        UINT64 Reserved8 : 7;

        /**
		 * @brief [Bit 40] When set to 1, the individual-address INVVPID type is supported.
		 */
        UINT64 InvvpidIndividualAddress : 1;

        /**
		 * @brief [Bit 41] When set to 1, the single-context INVVPID type is supported.
		 */
        UINT64 InvvpidSingleContext : 1;

        /**
		 * @brief [Bit 42] When set to 1, the all-context INVVPID type is supported.
		 */
        UINT64 InvvpidAllContexts : 1;

        /**
		 * @brief [Bit 43] When set to 1, the single-context-retaining-globals INVVPID type is supported.
		 */
        UINT64 InvvpidSingleContextRetainGlobals : 1;
        UINT64 Reserved9 : 20;
    };

    UINT64 Flags;
} IA32_VMX_EPT_VPID_CAP_REGISTER, *PIA32_VMX_EPT_VPID_CAP_REGISTER;

/**
 * @brief EPT PML4-E Structure
 * 
 */
typedef union _PEPT_PML4
{
    struct
    {
        /**
		 * @brief [Bit 0] Read access; indicates whether reads are allowed from the 512-GByte region controlled by this entry.
		 */
        UINT64 ReadAccess : 1;

        /**
		 * @brief [Bit 1] Write access; indicates whether writes are allowed from the 512-GByte region controlled by this entry.
		 */
        UINT64 WriteAccess : 1;

        /**
		 * @brief [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
		 * instruction fetches are allowed from the 512-GByte region controlled by this entry.
		 * If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
		 * allowed from supervisor-mode linear addresses in the 512-GByte region controlled by this entry.
		 */
        UINT64 ExecuteAccess : 1;
        UINT64 Reserved1 : 5;

        /**
		 * @brief [Bit 8] If bit 6 of EPTP is 1, accessed flag for EPT; indicates whether software has accessed the 512-GByte region
		 * controlled by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */
        UINT64 Accessed : 1;
        UINT64 Reserved2 : 1;

        /**
		 * @brief [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
		 * is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 512-GByte region
		 * controlled by this entry. If that control is 0, this bit is ignored.
		 */
        UINT64 UserModeExecute : 1;
        UINT64 Reserved3 : 1;

        /**
		 * @brief [Bits 47:12] Physical address of 4-KByte aligned EPT page-directory-pointer table referenced by this entry.
		 */
        UINT64 PageFrameNumber : 36;
        UINT64 Reserved4 : 16;
    };

    UINT64 Flags;
} EPT_PML4, *PEPT_PML4;

/**
 * @brief EPT PDPT Structure (1 GB)
 * 
 */
typedef union _EPDPTE_1GB
{
    struct
    {
        /**
		 * @brief [Bit 0] Read access; indicates whether reads are allowed from the 1-GByte page referenced by this entry.
		 */
        UINT64 ReadAccess : 1;

        /**
		 * @brief [Bit 1] Write access; indicates whether writes are allowed from the 1-GByte page referenced by this entry.
		 */
        UINT64 WriteAccess : 1;

        /**
		 * @brief [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
		 * instruction fetches are allowed from the 1-GByte page controlled by this entry.
		 * If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
		 * allowed from supervisor-mode linear addresses in the 1-GByte page controlled by this entry.
		 */
        UINT64 ExecuteAccess : 1;

        /**
		 * @brief [Bits 5:3] EPT memory type for this 1-GByte page.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */
        UINT64 MemoryType : 3;

        /**
		 * @brief [Bit 6] Ignore PAT memory type for this 1-GByte page.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */
        UINT64 IgnorePat : 1;

        /**
		 * @brief [Bit 7] Must be 1 (otherwise, this entry references an EPT page directory).
		 */
        UINT64 LargePage : 1;

        /**
		 * @brief [Bit 8] If bit 6 of EPTP is 1, accessed flag for EPT; indicates whether software has accessed the 1-GByte page
		 * referenced by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */
        UINT64 Accessed : 1;

        /**
		 * @brief [Bit 9] If bit 6 of EPTP is 1, dirty flag for EPT; indicates whether software has written to the 1-GByte page referenced
		 * by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */
        UINT64 Dirty : 1;

        /**
		 * @brief [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
		 * is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 1-GByte page controlled
		 * by this entry. If that control is 0, this bit is ignored.
		 */
        UINT64 UserModeExecute : 1;
        UINT64 Reserved1 : 19;

        /**
		 * @brief [Bits 47:30] Physical address of 4-KByte aligned EPT page-directory-pointer table referenced by this entry.
		 */
        UINT64 PageFrameNumber : 18;
        UINT64 Reserved2 : 15;

        /**
		 * @brief [Bit 63] Suppress \#VE. If the "EPT-violation \#VE" VM-execution control is 1, EPT violations caused by accesses to this
		 * page are convertible to virtualization exceptions only if this bit is 0. If "EPT-violation \#VE" VMexecution control is
		 * 0, this bit is ignored.
		 *
		 * @see Vol3C[25.5.6.1(Convertible EPT Violations)]
		 */
        UINT64 SuppressVe : 1;
    };

    UINT64 Flags;
} EPDPTE_1GB, *PEPDPTE_1GB;

/**
 * @brief EPT PDPT-E Structure
 * 
 */
typedef union _EPDPTE
{
    struct
    {
        /**
		 * @brief [Bit 0] Read access; indicates whether reads are allowed from the 1-GByte region controlled by this entry.
		 */
        UINT64 ReadAccess : 1;

        /**
		 * @brief [Bit 1] Write access; indicates whether writes are allowed from the 1-GByte region controlled by this entry.
		 */
        UINT64 WriteAccess : 1;

        /**
		 * @brief [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
		 * instruction fetches are allowed from the 1-GByte region controlled by this entry.
		 * If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
		 * allowed from supervisor-mode linear addresses in the 1-GByte region controlled by this entry.
		 */
        UINT64 ExecuteAccess : 1;
        UINT64 Reserved1 : 5;

        /**
		 * @brief [Bit 8] If bit 6 of EPTP is 1, accessed flag for EPT; indicates whether software has accessed the 1-GByte region
		 * controlled by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */
        UINT64 Accessed : 1;
        UINT64 Reserved2 : 1;

        /**
		 * @brief [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
		 * is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 1-GByte region controlled
		 * by this entry. If that control is 0, this bit is ignored.
		 */
        UINT64 UserModeExecute : 1;
        UINT64 Reserved3 : 1;

        /**
		 * @brief [Bits 47:12] Physical address of 4-KByte aligned EPT page-directory-pointer table referenced by this entry.
		 */
        UINT64 PageFrameNumber : 36;
        UINT64 Reserved4 : 16;
    };

    UINT64 Flags;
} EPDPTE, *PEPDPTE;

/**
 * @brief EPT PD-E (2MB)
 * 
 */
typedef union _EPDE_2MB
{
    struct
    {
        /**
		 * @brief [Bit 0] Read access; indicates whether reads are allowed from the 2-MByte page referenced by this entry.
		 */
        UINT64 ReadAccess : 1;

        /**
		 * @brief [Bit 1] Write access; indicates whether writes are allowed from the 2-MByte page referenced by this entry.
		 */
        UINT64 WriteAccess : 1;

        /**
		 * @brief [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
		 * instruction fetches are allowed from the 2-MByte page controlled by this entry.
		 * If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
		 * allowed from supervisor-mode linear addresses in the 2-MByte page controlled by this entry.
		 */
        UINT64 ExecuteAccess : 1;

        /**
		 * @brief [Bits 5:3] EPT memory type for this 2-MByte page.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */
        UINT64 MemoryType : 3;

        /**
		 * @brief [Bit 6] Ignore PAT memory type for this 2-MByte page.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */
        UINT64 IgnorePat : 1;

        /**
		 * @brief [Bit 7] Must be 1 (otherwise, this entry references an EPT page table).
		 */
        UINT64 LargePage : 1;

        /**
		 * @brief [Bit 8] If bit 6 of EPTP is 1, accessed flag for EPT; indicates whether software has accessed the 2-MByte page
		 * referenced by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */
        UINT64 Accessed : 1;

        /**
		 * @brief [Bit 9] If bit 6 of EPTP is 1, dirty flag for EPT; indicates whether software has written to the 2-MByte page referenced
		 * by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */
        UINT64 Dirty : 1;

        /**
		 * @brief [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
		 * is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 2-MByte page controlled
		 * by this entry. If that control is 0, this bit is ignored.
		 */
        UINT64 UserModeExecute : 1;
        UINT64 Reserved1 : 10;

        /**
		 * @brief [Bits 47:21] Physical address of 4-KByte aligned EPT page-directory-pointer table referenced by this entry.
		 */
        UINT64 PageFrameNumber : 27;
        UINT64 Reserved2 : 15;

        /**
		 * @brief [Bit 63] Suppress \#VE. If the "EPT-violation \#VE" VM-execution control is 1, EPT violations caused by accesses to this
		 * page are convertible to virtualization exceptions only if this bit is 0. If "EPT-violation \#VE" VMexecution control is
		 * 0, this bit is ignored.
		 *
		 * @see Vol3C[25.5.6.1(Convertible EPT Violations)]
		 */
        UINT64 SuppressVe : 1;
    };

    UINT64 Flags;
} EPDE_2MB, *PEPDE_2MB;

/**
 * @brief EPT PDE Structure
 * 
 */
typedef union _EPDE
{
    struct
    {
        /**
		 * @brief [Bit 0] Read access; indicates whether reads are allowed from the 2-MByte region controlled by this entry.
		 */
        UINT64 ReadAccess : 1;

        /**
		 * @brief [Bit 1] Write access; indicates whether writes are allowed from the 2-MByte region controlled by this entry.
		 */
        UINT64 WriteAccess : 1;

        /**
		 * @brief [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
		 * instruction fetches are allowed from the 2-MByte region controlled by this entry.
		 * If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
		 * allowed from supervisor-mode linear addresses in the 2-MByte region controlled by this entry.
		 */
        UINT64 ExecuteAccess : 1;
        UINT64 Reserved1 : 5;

        /**
		 * @brief [Bit 8] If bit 6 of EPTP is 1, accessed flag for EPT; indicates whether software has accessed the 2-MByte region
		 * controlled by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */
        UINT64 Accessed : 1;
        UINT64 Reserved2 : 1;

        /**
		 * @brief [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
		 * is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 2-MByte region controlled
		 * by this entry. If that control is 0, this bit is ignored.
		 */
        UINT64 UserModeExecute : 1;
        UINT64 Reserved3 : 1;

        /**
		 * @brief [Bits 47:12] Physical address of 4-KByte aligned EPT page table referenced by this entry.
		 */
        UINT64 PageFrameNumber : 36;
        UINT64 Reserved4 : 16;
    };

    UINT64 Flags;
} EPDE, *PEPDE;

/**
 * @brief EPT PTE
 * 
 */
typedef union _EPTE
{
    struct
    {
        /**
		 * @brief [Bit 0] Read access; indicates whether reads are allowed from the 4-KByte page referenced by this entry.
		 */
        UINT64 ReadAccess : 1;

        /**
		 * @brief [Bit 1] Write access; indicates whether writes are allowed from the 4-KByte page referenced by this entry.
		 */
        UINT64 WriteAccess : 1;

        /**
		 * @brief [Bit 2] If the "mode-based execute control for EPT" VM-execution control is 0, execute access; indicates whether
		 * instruction fetches are allowed from the 4-KByte page controlled by this entry.
		 * If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction fetches are
		 * allowed from supervisor-mode linear addresses in the 4-KByte page controlled by this entry.
		 */
        UINT64 ExecuteAccess : 1;

        /**
		 * @brief [Bits 5:3] EPT memory type for this 4-KByte page.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */
        UINT64 MemoryType : 3;

        /**
		 * @brief [Bit 6] Ignore PAT memory type for this 4-KByte page.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */
        UINT64 IgnorePat : 1;
        UINT64 Reserved1 : 1;

        /**
		 * @brief [Bit 8] If bit 6 of EPTP is 1, accessed flag for EPT; indicates whether software has accessed the 4-KByte page
		 * referenced by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */
        UINT64 Accessed : 1;

        /**
		 * @brief [Bit 9] If bit 6 of EPTP is 1, dirty flag for EPT; indicates whether software has written to the 4-KByte page referenced
		 * by this entry. Ignored if bit 6 of EPTP is 0.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */
        UINT64 Dirty : 1;

        /**
		 * @brief [Bit 10] Execute access for user-mode linear addresses. If the "mode-based execute control for EPT" VM-execution control
		 * is 1, indicates whether instruction fetches are allowed from user-mode linear addresses in the 4-KByte page controlled
		 * by this entry. If that control is 0, this bit is ignored.
		 */
        UINT64 UserModeExecute : 1;
        UINT64 Reserved2 : 1;

        /**
		 * @brief [Bits 47:12] Physical address of the 4-KByte page referenced by this entry.
		 */
        UINT64 PageFrameNumber : 36;
        UINT64 Reserved3 : 15;

        /**
		 * @brief [Bit 63] Suppress \#VE. If the "EPT-violation \#VE" VM-execution control is 1, EPT violations caused by accesses to this
		 * page are convertible to virtualization exceptions only if this bit is 0. If "EPT-violation \#VE" VMexecution control is
		 * 0, this bit is ignored.
		 *
		 * @see Vol3C[25.5.6.1(Convertible EPT Violations)]
		 */
        UINT64 SuppressVe : 1;
    };

    UINT64 Flags;
} EPTE, *PEPTE;

//////////////////////////////////////////////////
//				      typedefs         			 //
//////////////////////////////////////////////////

typedef EPT_PML4 EPT_PML4_POINTER, *PEPT_PML4_POINTER;
typedef EPDPTE   EPT_PML3_POINTER, *PEPT_PML3_POINTER;
typedef EPDE_2MB EPT_PML2_ENTRY, *PEPT_PML2_ENTRY;
typedef EPDE     EPT_PML2_POINTER, *PEPT_PML2_POINTER;
typedef EPTE     EPT_PML1_ENTRY, *PEPT_PML1_ENTRY;

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
 * @brief EPT Pointer (EPTP or GUEST_EPTP)
 * 
 */
typedef union _EPTP
{
    struct
    {
        /**
		 * @brief [Bits 2:0] EPT paging-structure memory type:
		 * - 0 = Uncacheable (UC)
		 * - 6 = Write-back (WB)
		 * Other values are reserved.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */
        UINT64 MemoryType : 3;

        /**
		 * @brief [Bits 5:3] This value is 1 less than the EPT page-walk length.
		 *
		 * @see Vol3C[28.2.6(EPT and memory Typing)]
		 */
        UINT64 PageWalkLength : 3;

        /**
		 * @brief [Bit 6] Setting this control to 1 enables accessed and dirty flags for EPT.
		 *
		 * @see Vol3C[28.2.4(Accessed and Dirty Flags for EPT)]
		 */
        UINT64 EnableAccessAndDirtyFlags : 1;
        UINT64 Reserved1 : 5;

        /**
		 * @brief [Bits 47:12] Bits N-1:12 of the physical address of the 4-KByte aligned EPT PML4 table.
		 */
        UINT64 PageFrameNumber : 36;
        UINT64 Reserved2 : 16;
    };

    UINT64 Flags;
} EPTP, *PEPTP;

/**
 * @brief MSR_IA32_MTRR_DEF_TYPE Structure
 * 
 */
typedef union _IA32_MTRR_DEF_TYPE_REGISTER
{
    struct
    {
        /**
		 * @brief [Bits 2:0] Default Memory Type.
		 */
        UINT64 DefaultMemoryType : 3;
        UINT64 Reserved1 : 7;

        /**
		 * @brief [Bit 10] Fixed Range MTRR Enable.
		 */
        UINT64 FixedRangeMtrrEnable : 1;

        /**
		 * @brief [Bit 11] MTRR Enable.
		 */
        UINT64 MtrrEnable : 1;
        UINT64 Reserved2 : 52;
    };

    UINT64 Flags;
} IA32_MTRR_DEF_TYPE_REGISTER, *PIA32_MTRR_DEF_TYPE_REGISTER;

/**
 * @brief IA32_MTRR_CAPABILITIES Structure
 * 
 */
typedef union _IA32_MTRR_CAPABILITIES_REGISTER
{
    struct
    {
        /**
		 * @brief VCNT (variable range registers count) field
		 *
		 * [Bits 7:0] Indicates the number of variable ranges implemented on the processor.
		 */
        UINT64 VariableRangeCount : 8;

        /**
		 * @brief FIX (fixed range registers supported) flag
		 *
		 * [Bit 8] Fixed range MTRRs (MSR_IA32_MTRR_FIX64K_00000 through MSR_IA32_MTRR_FIX4K_0F8000) are supported when set; no fixed range
		 * registers are supported when clear.
		 */
        UINT64 FixedRangeSupported : 1;
        UINT64 Reserved1 : 1;

        /**
		 * @brief WC (write combining) flag
		 *
		 * [Bit 10] The write-combining (WC) memory type is supported when set; the WC type is not supported when clear.
		 */
        UINT64 WcSupported : 1;

        /**
		 * @brief SMRR (System-Management Range Register) flag
		 *
		 * [Bit 11] The system-management range register (SMRR) interface is supported when bit 11 is set; the SMRR interface is
		 * not supported when clear.
		 */
        UINT64 SmrrSupported : 1;
        UINT64 Reserved2 : 52;
    };

    UINT64 Flags;
} IA32_MTRR_CAPABILITIES_REGISTER, *PIA32_MTRR_CAPABILITIES_REGISTER;

/**
 * @brief MSR_IA32_MTRR_PHYSBASE(0-9) Structure
 * 
 */
typedef union _IA32_MTRR_PHYSBASE_REGISTER
{
    struct
    {
        /**
		 * @brief [Bits 7:0] Specifies the memory type for the range.
		 */
        UINT64 Type : 8;
        UINT64 Reserved1 : 4;

        /**
		 * @brief [Bits 47:12] Specifies the base address of the address range. This 24-bit value, in the case where MAXPHYADDR is 36
		 * bits, is extended by 12 bits at the low end to form the base address (this automatically aligns the address on a 4-KByte
		 * boundary).
		 */
        UINT64 PageFrameNumber : 36;
        UINT64 Reserved2 : 16;
    };

    UINT64 Flags;
} IA32_MTRR_PHYSBASE_REGISTER, *PIA32_MTRR_PHYSBASE_REGISTER;

/**
 * @brief MSR_IA32_MTRR_PHYSMASK(0-9) structure
 * 
 */
typedef union _IA32_MTRR_PHYSMASK_REGISTER
{
    struct
    {
        /**
		 * @brief [Bits 7:0] Specifies the memory type for the range.
		 */
        UINT64 Type : 8;
        UINT64 Reserved1 : 3;

        /**
		 * @brief [Bit 11] Enables the register pair when set; disables register pair when clear.
		 */
        UINT64 Valid : 1;

        /**
		 * @brief [Bits 47:12] Specifies a mask (24 bits if the maximum physical address size is 36 bits, 28 bits if the maximum physical
		 * address size is 40 bits). The mask determines the range of the region being mapped, according to the following
		 * relationships:
		 * - Address_Within_Range AND PhysMask = PhysBase AND PhysMask
		 * - This value is extended by 12 bits at the low end to form the mask value.
		 * - The width of the PhysMask field depends on the maximum physical address size supported by the processor.
		 * CPUID.80000008H reports the maximum physical address size supported by the processor. If CPUID.80000008H is not
		 * available, software may assume that the processor supports a 36-bit physical address size.
		 *
		 * @see Vol3A[11.11.3(Example Base and Mask Calculations)]
		 */
        UINT64 PageFrameNumber : 36;
        UINT64 Reserved2 : 16;
    };

    UINT64 Flags;
} IA32_MTRR_PHYSMASK_REGISTER, *PIA32_MTRR_PHYSMASK_REGISTER;

/**
 * @brief Structure for INVEPT Instruction
 * 
 */
typedef struct _INVEPT_DESCRIPTOR
{
    UINT64 EptPointer;
    UINT64 Reserved; // Must be zero.
} INVEPT_DESCRIPTOR, *PINVEPT_DESCRIPTOR;

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
typedef struct _EPT_STATE
{
    LIST_ENTRY            HookedPagesList;             // A list of the details about hooked pages
    MTRR_RANGE_DESCRIPTOR MemoryRanges[9];             // Physical memory ranges described by the BIOS in the MTRRs. Used to build the EPT identity mapping.
    ULONG                 NumberOfEnabledMemoryRanges; // Number of memory ranges specified in MemoryRanges
    EPTP                  EptPointer;                  // Extended-Page-Table Pointer
    PVMM_EPT_PAGE_TABLE   EptPageTable;                // Page table entries for EPT operation

    PVMM_EPT_PAGE_TABLE SecondaryEptPageTable; // Secondary Page table entries for EPT operation (Used in debugger mechanisms)
    BOOLEAN             SecondaryInitialized;  // Is Secondary Page table entries initialized or not (Used in debugger mechanisms)
    EPTP                SecondaryEptPointer;   // Secondary Extended-Page-Table Pointer

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
 * @brief Stucture of EPT Violation's Exit Qualification
 * 
 */
typedef union _VMX_EXIT_QUALIFICATION_EPT_VIOLATION
{
    struct
    {
        /**
		 * @brief [Bit 0] Set if the access causing the EPT violation was a data read.
		 */
        UINT64 ReadAccess : 1;

        /**
		 * @brief [Bit 1] Set if the access causing the EPT violation was a data write.
		 */
        UINT64 WriteAccess : 1;

        /**
		 * @brief [Bit 2] Set if the access causing the EPT violation was an instruction fetch.
		 */
        UINT64 ExecuteAccess : 1;

        /**
		 * @brief [Bit 3] The logical-AND of bit 0 in the EPT paging-structure entries used to translate the guest-physical address of the
		 * access causing the EPT violation (indicates whether the guest-physical address was readable).
		 */
        UINT64 EptReadable : 1;

        /**
		 * @brief [Bit 4] The logical-AND of bit 1 in the EPT paging-structure entries used to translate the guest-physical address of the
		 * access causing the EPT violation (indicates whether the guest-physical address was writeable).
		 */
        UINT64 EptWriteable : 1;

        /**
		 * @brief [Bit 5] The logical-AND of bit 2 in the EPT paging-structure entries used to translate the guest-physical address of the
		 * access causing the EPT violation.
		 * If the "mode-based execute control for EPT" VM-execution control is 0, this indicates whether the guest-physical address
		 * was executable. If that control is 1, this indicates whether the guest-physical address was executable for
		 * supervisor-mode linear addresses.
		 */
        UINT64 EptExecutable : 1;

        /**
		 * @brief [Bit 6] If the "mode-based execute control" VM-execution control is 0, the value of this bit is undefined. If that
		 * control is 1, this bit is the logical-AND of bit 10 in the EPT paging-structures entries used to translate the
		 * guest-physical address of the access causing the EPT violation. In this case, it indicates whether the guest-physical
		 * address was executable for user-mode linear addresses.
		 */
        UINT64 EptExecutableForUserMode : 1;

        /**
		 * @brief [Bit 7] Set if the guest linear-address field is valid. The guest linear-address field is valid for all EPT violations
		 * except those resulting from an attempt to load the guest PDPTEs as part of the execution of the MOV CR instruction.
		 */
        UINT64 ValidGuestLinearAddress : 1;

        /**
		 * @brief [Bit 8] If bit 7 is 1:
		 * - Set if the access causing the EPT violation is to a guest-physical address that is the translation of a linear
		 * address.
		 * - Clear if the access causing the EPT violation is to a paging-structure entry as part of a page walk or the update of
		 * an accessed or dirty bit.
		 * Reserved if bit 7 is 0 (cleared to 0).
		 */
        UINT64 CausedByTranslation : 1;

        /**
		 * @brief [Bit 9] This bit is 0 if the linear address is a supervisor-mode linear address and 1 if it is a user-mode linear
		 * address. Otherwise, this bit is undefined.
		 *
		 * @remarks If bit 7 is 1, bit 8 is 1, and the processor supports advanced VM-exit information for EPT violations. (If
		 *          CR0.PG = 0, the translation of every linear address is a user-mode linear address and thus this bit will be 1.)
		 */
        UINT64 UserModeLinearAddress : 1;

        /**
		 * @brief [Bit 10] This bit is 0 if paging translates the linear address to a read-only page and 1 if it translates to a
		 * read/write page. Otherwise, this bit is undefined
		 *
		 * @remarks If bit 7 is 1, bit 8 is 1, and the processor supports advanced VM-exit information for EPT violations. (If
		 *          CR0.PG = 0, every linear address is read/write and thus this bit will be 1.)
		 */
        UINT64 ReadableWritablePage : 1;

        /**
		 * @brief [Bit 11] This bit is 0 if paging translates the linear address to an executable page and 1 if it translates to an
		 * execute-disable page. Otherwise, this bit is undefined.
		 *
		 * @remarks If bit 7 is 1, bit 8 is 1, and the processor supports advanced VM-exit information for EPT violations. (If
		 *          CR0.PG = 0, CR4.PAE = 0, or MSR_IA32_EFER.NXE = 0, every linear address is executable and thus this bit will be 0.)
		 */
        UINT64 ExecuteDisablePage : 1;

        /**
		 * @brief [Bit 12] NMI unblocking due to IRET.
		 */
        UINT64 NmiUnblocking : 1;
        UINT64 Reserved1 : 51;
    };

    UINT64 Flags;
} VMX_EXIT_QUALIFICATION_EPT_VIOLATION, *PVMX_EXIT_QUALIFICATION_EPT_VIOLATION;

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

/**
 * @brief INVEPT Enum
 * 
 */
typedef enum _INVEPT_TYPE
{
    INVEPT_SINGLE_CONTEXT = 0x00000001,
    INVEPT_ALL_CONTEXTS   = 0x00000002
} INVEPT_TYPE;

//////////////////////////////////////////////////
//				    Functions					//
//////////////////////////////////////////////////

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
EptSplitLargePage(PVMM_EPT_PAGE_TABLE EptPageTable, PVOID PreAllocatedBuffer, SIZE_T PhysicalAddress, ULONG CoreIndex);

/**
 * @brief Initialize EPT Table based on Processor Index
 * 
 * @return BOOLEAN 
 */
BOOLEAN
EptLogicalProcessorInitialize();

/**
 * @brief Initialize the secondary EPT Table based on Processor Index
 * 
 * @return BOOLEAN 
 */
BOOLEAN
EptInitializeSeconadaryEpt();

/**
 * @brief Handle EPT Violation
 * 
 * @param Regs 
 * @param ExitQualification 
 * @param GuestPhysicalAddr 
 * @return BOOLEAN 
 */
BOOLEAN
EptHandleEptViolation(PGUEST_REGS Regs, ULONG ExitQualification, UINT64 GuestPhysicalAddr);

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
EptSetPML1AndInvalidateTLB(PEPT_PML1_ENTRY EntryAddress, EPT_PML1_ENTRY EntryValue, INVEPT_TYPE InvalidationType);
