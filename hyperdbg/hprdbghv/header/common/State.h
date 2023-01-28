/**
 * @file State.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Model-Specific Registers definitions
 *
 * @version 0.2
 * @date 2022-12-01
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				      typedefs         			 //
//////////////////////////////////////////////////

typedef EPT_PML4E   EPT_PML4_POINTER, *PEPT_PML4_POINTER;
typedef EPT_PDPTE   EPT_PML3_POINTER, *PEPT_PML3_POINTER;
typedef EPT_PDE_2MB EPT_PML2_ENTRY, *PEPT_PML2_ENTRY;
typedef EPT_PDE     EPT_PML2_POINTER, *PEPT_PML2_POINTER;
typedef EPT_PTE     EPT_PML1_ENTRY, *PEPT_PML1_ENTRY;

//////////////////////////////////////////////////
//				    Constants					//
//////////////////////////////////////////////////

/**
 * @brief Pending External Interrups Buffer Capacity
 *
 */
#define PENDING_INTERRUPTS_BUFFER_CAPACITY 64

/**
 * @brief Maximum number of hidden breakpoints in a page
 *
 */
#define MaximumHiddenBreakpointsOnPage 40

//////////////////////////////////////////////////
//					  Enums		    			//
//////////////////////////////////////////////////

/**
 * @brief Types of actions for NMI broadcasting
 *
 */
typedef enum _NMI_BROADCAST_ACTION_TYPE
{
    NMI_BROADCAST_ACTION_NONE = 0,
    NMI_BROADCAST_ACTION_TEST,
    NMI_BROADCAST_ACTION_REQUEST,

} NMI_BROADCAST_ACTION_TYPE;

//////////////////////////////////////////////////
//					  Structure	    			//
//////////////////////////////////////////////////

/**
 * @brief The status of transparency of each core after and before VMX
 *
 */
typedef struct _VM_EXIT_TRANSPARENCY
{
    UINT64 PreviousTimeStampCounter;

    HANDLE  ThreadId;
    UINT64  RevealedTimeStampCounterByRdtsc;
    BOOLEAN CpuidAfterRdtscDetected;

} VM_EXIT_TRANSPARENCY, *PVM_EXIT_TRANSPARENCY;

/**
 * @brief Save the state of core in the case of VMXOFF
 *
 */
typedef struct _VMX_VMXOFF_STATE
{
    BOOLEAN IsVmxoffExecuted; // Shows whether the VMXOFF executed or not
    UINT64  GuestRip;         // Rip address of guest to return
    UINT64  GuestRsp;         // Rsp address of guest to return

} VMX_VMXOFF_STATE, *PVMX_VMXOFF_STATE;

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
     * @brief If TRUE, this hook relates to the write violation of the events
     */
    BOOLEAN IsMonitorToWriteOnPages;

    /**
     * @brief Temporary context for the post event monitors
     * It shows the context of the last address that triggered the hook
     * Note: Only used for read/write trigger events
     */
    EPT_HOOKS_CONTEXT LastContextState;

    /**
     * @brief This field shows whether the hook should call the post event trigger
     * after restoring the state or not
     */
    BOOLEAN IsPostEventTriggerAllowed;

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

/**
 * @brief The status of NMI broadcasting in VMX
 *
 */
typedef struct _NMI_BROADCASTING_STATE
{
    volatile NMI_BROADCAST_ACTION_TYPE NmiBroadcastAction; // The broadcast action for NMI

} NMI_BROADCASTING_STATE, *PNMI_BROADCASTING_STATE;

/**
 * @brief The status of each core after and before VMX
 *
 */
typedef struct _VIRTUAL_MACHINE_STATE
{
    BOOLEAN      IsOnVmxRootMode;                                               // Detects whether the current logical core is on Executing on VMX Root Mode
    BOOLEAN      IncrementRip;                                                  // Checks whether it has to redo the previous instruction or not (it used mainly in Ept routines)
    BOOLEAN      HasLaunched;                                                   // Indicate whether the core is virtualized or not
    BOOLEAN      IgnoreMtfUnset;                                                // Indicate whether the core should ignore unsetting the MTF or not
    BOOLEAN      WaitForImmediateVmexit;                                        // Whether the current core is waiting for an immediate vm-exit or not
    BOOLEAN      EnableExternalInterruptsOnContinue;                            // Whether to enable external interrupts on the continue  or not
    BOOLEAN      EnableExternalInterruptsOnContinueMtf;                         // Whether to enable external interrupts on the continue state of MTF or not
    BOOLEAN      RegisterBreakOnMtf;                                            // Registered Break in the case of MTFs (used in instrumentation step-in)
    BOOLEAN      IgnoreOneMtf;                                                  // Ignore (mark as handled) for one MTF
    GUEST_REGS * Regs;                                                          // The virtual processor's general-purpose registers
    UINT32       CoreId;                                                        // The core's unique identifier
    ULONG        ExitReason;                                                    // The core's exit reason
    UINT32       ExitQualification;                                             // The core's exit qualification
    UINT64       LastVmexitRip;                                                 // RIP in the current VM-exit
    UINT64       VmxonRegionPhysicalAddress;                                    // Vmxon region physical address
    UINT64       VmxonRegionVirtualAddress;                                     // VMXON region virtual address
    UINT64       VmcsRegionPhysicalAddress;                                     // VMCS region physical address
    UINT64       VmcsRegionVirtualAddress;                                      // VMCS region virtual address
    UINT64       VmmStack;                                                      // Stack for VMM in VM-Exit State
    UINT64       MsrBitmapVirtualAddress;                                       // Msr Bitmap Virtual Address
    UINT64       MsrBitmapPhysicalAddress;                                      // Msr Bitmap Physical Address
    UINT64       IoBitmapVirtualAddressA;                                       // I/O Bitmap Virtual Address (A)
    UINT64       IoBitmapPhysicalAddressA;                                      // I/O Bitmap Physical Address (A)
    UINT64       IoBitmapVirtualAddressB;                                       // I/O Bitmap Virtual Address (B)
    UINT64       IoBitmapPhysicalAddressB;                                      // I/O Bitmap Physical Address (B)
    UINT32       PendingExternalInterrupts[PENDING_INTERRUPTS_BUFFER_CAPACITY]; // This list holds a buffer for external-interrupts that are in pending state due to the external-interrupt
                                                                                // blocking and waits for interrupt-window exiting
                                                                                // From hvpp :
                                                                                // Pending interrupt queue (FIFO).
                                                                                // Make storage for up-to 64 pending interrupts.
                                                                                // In practice I haven't seen more than 2 pending interrupts.

    VMX_VMXOFF_STATE        VmxoffState;            // Shows the vmxoff state of the guest
    NMI_BROADCASTING_STATE  NmiBroadcastingState;   // Shows the state of NMI broadcasting
    VM_EXIT_TRANSPARENCY    TransparencyState;      // The state of the debugger in transparent-mode
    PEPT_HOOKED_PAGE_DETAIL MtfEptHookRestorePoint; // It shows the detail of the hooked paged that should be restore in MTF vm-exit

} VIRTUAL_MACHINE_STATE, *PVIRTUAL_MACHINE_STATE;
