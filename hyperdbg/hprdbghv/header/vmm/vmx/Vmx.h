/**
 * @file Vmx.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief VMX Instruction and operation headers
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
 * @brief VMCS Region Size
 * 
 */
#define VMCS_SIZE 4096

/**
 * @brief VMXON Region Size
 * 
 */
#define VMXON_SIZE 4096

/**
 * @brief PIN-Based Execution
 * 
 */
#define PIN_BASED_VM_EXECUTION_CONTROLS_EXTERNAL_INTERRUPT        0x00000001
#define PIN_BASED_VM_EXECUTION_CONTROLS_NMI_EXITING               0x00000008
#define PIN_BASED_VM_EXECUTION_CONTROLS_VIRTUAL_NMI               0x00000020
#define PIN_BASED_VM_EXECUTION_CONTROLS_ACTIVE_VMX_TIMER          0x00000040
#define PIN_BASED_VM_EXECUTION_CONTROLS_PROCESS_POSTED_INTERRUPTS 0x00000080

/**
 * @brief CPU-Based Controls
 * 
 */
#define CPU_BASED_VIRTUAL_INTR_PENDING        0x00000004
#define CPU_BASED_USE_TSC_OFFSETING           0x00000008
#define CPU_BASED_HLT_EXITING                 0x00000080
#define CPU_BASED_INVLPG_EXITING              0x00000200
#define CPU_BASED_MWAIT_EXITING               0x00000400
#define CPU_BASED_RDPMC_EXITING               0x00000800
#define CPU_BASED_RDTSC_EXITING               0x00001000
#define CPU_BASED_CR3_LOAD_EXITING            0x00008000
#define CPU_BASED_CR3_STORE_EXITING           0x00010000
#define CPU_BASED_CR8_LOAD_EXITING            0x00080000
#define CPU_BASED_CR8_STORE_EXITING           0x00100000
#define CPU_BASED_TPR_SHADOW                  0x00200000
#define CPU_BASED_VIRTUAL_NMI_PENDING         0x00400000
#define CPU_BASED_MOV_DR_EXITING              0x00800000
#define CPU_BASED_UNCOND_IO_EXITING           0x01000000
#define CPU_BASED_ACTIVATE_IO_BITMAP          0x02000000
#define CPU_BASED_MONITOR_TRAP_FLAG           0x08000000
#define CPU_BASED_ACTIVATE_MSR_BITMAP         0x10000000
#define CPU_BASED_MONITOR_EXITING             0x20000000
#define CPU_BASED_PAUSE_EXITING               0x40000000
#define CPU_BASED_ACTIVATE_SECONDARY_CONTROLS 0x80000000

/**
 * @brief Secondary CPU-Based Controls
 * 
 */
#define CPU_BASED_CTL2_ENABLE_EPT                 0x2
#define CPU_BASED_CTL2_RDTSCP                     0x8
#define CPU_BASED_CTL2_ENABLE_VPID                0x20
#define CPU_BASED_CTL2_UNRESTRICTED_GUEST         0x80
#define CPU_BASED_CTL2_VIRTUAL_INTERRUPT_DELIVERY 0x200
#define CPU_BASED_CTL2_ENABLE_INVPCID             0x1000
#define CPU_BASED_CTL2_ENABLE_VMFUNC              0x2000
#define CPU_BASED_CTL2_ENABLE_XSAVE_XRSTORS       0x100000

/**
 * @brief VM-exit Control Bits
 * 
 */
#define VM_EXIT_SAVE_DEBUG_CONTROLS        0x00000004
#define VM_EXIT_HOST_ADDR_SPACE_SIZE       0x00000200
#define VM_EXIT_LOAD_IA32_PERF_GLOBAL_CTRL 0x00001000
#define VM_EXIT_ACK_INTR_ON_EXIT           0x00008000
#define VM_EXIT_SAVE_IA32_PAT              0x00040000
#define VM_EXIT_LOAD_IA32_PAT              0x00080000
#define VM_EXIT_SAVE_IA32_EFER             0x00100000
#define VM_EXIT_LOAD_IA32_EFER             0x00200000
#define VM_EXIT_SAVE_VMX_PREEMPTION_TIMER  0x00400000

/**
 * @brief VM-entry Control Bits
 * 
 */
#define VM_ENTRY_LOAD_DEBUG_CONTROLS        0x00000004
#define VM_ENTRY_IA32E_MODE                 0x00000200
#define VM_ENTRY_SMM                        0x00000400
#define VM_ENTRY_DEACT_DUAL_MONITOR         0x00000800
#define VM_ENTRY_LOAD_IA32_PERF_GLOBAL_CTRL 0x00002000
#define VM_ENTRY_LOAD_IA32_PAT              0x00004000
#define VM_ENTRY_LOAD_IA32_EFER             0x00008000

/**
 * @brief CPUID RCX(s) - Based on Hyper-V
 * 
 */
#define HYPERV_CPUID_VENDOR_AND_MAX_FUNCTIONS 0x40000000
#define HYPERV_CPUID_INTERFACE                0x40000001
#define HYPERV_CPUID_VERSION                  0x40000002
#define HYPERV_CPUID_FEATURES                 0x40000003
#define HYPERV_CPUID_ENLIGHTMENT_INFO         0x40000004
#define HYPERV_CPUID_IMPLEMENT_LIMITS         0x40000005
#define HYPERV_HYPERVISOR_PRESENT_BIT         0x80000000
#define HYPERV_CPUID_MIN                      0x40000005
#define HYPERV_CPUID_MAX                      0x4000ffff

/**
 * @brief GUEST_INTERRUPTIBILITY_INFO flags
 * 
 */
#define GUEST_INTR_STATE_STI          0x00000001
#define GUEST_INTR_STATE_MOV_SS       0x00000002
#define GUEST_INTR_STATE_SMI          0x00000004
#define GUEST_INTR_STATE_NMI          0x00000008
#define GUEST_INTR_STATE_ENCLAVE_INTR 0x00000010

/**
 * @brief Interrupt shadow states
 * 
 */
#define SHADOW_INT_MOV_SS 0x01
#define SHADOW_INT_STI    0x02

/**
 * @brief Stack Size
 * 
 */
#define VMM_STACK_SIZE 0x8000

/**
 * @brief Pending External Interrups Buffer Capacity
 * 
 */
#define PENDING_INTERRUPTS_BUFFER_CAPACITY 64

#define IS_VALID_DEBUG_REGISTER(DebugRegister)                   \
    (((DebugRegister <= VMX_EXIT_QUALIFICATION_REGISTER_DR0) &&  \
      (DebugRegister <= VMX_EXIT_QUALIFICATION_REGISTER_DR7)) && \
     (DebugRegister != 0x00000004 && DebugRegister != 0x00000005))

/**
 * @brief Hypercalls for Hyper-V
 * 
 */
typedef union _HYPERCALL_INPUT_VALUE
{
    UINT64 Flags;
    struct
    {
        UINT64 CallCode : 16; // HYPERCALL_CODE
        UINT64 Fast : 1;
        UINT64 VariableHeaderSize : 9;
        UINT64 IsNested : 1;
        UINT64 Reserved0 : 5;
        UINT64 RepCount : 12;
        UINT64 Reserved1 : 4;
        UINT64 RepStartIndex : 12;
        UINT64 Reserved2 : 4;
    } Fields;

} HYPERCALL_INPUT_VALUE, *PHYPERCALL_INPUT_VALUE;

/**
 * @brief Hyper-V Hypercalls
 * 
 */
enum HYPERCALL_CODE
{
    HvSwitchVirtualAddressSpace  = 0x0001,
    HvFlushVirtualAddressSpace   = 0x0002,
    HvFlushVirtualAddressList    = 0x0003,
    HvGetLogicalProcessorRunTime = 0x0004,
    // 0x0005..0x0007 are reserved
    HvCallNotifyLongSpinWait         = 0x0008,
    HvCallParkedVirtualProcessors    = 0x0009,
    HvCallSyntheticClusterIpi        = 0x000B,
    HvCallModifyVtlProtectionMask    = 0x000C,
    HvCallEnablePartitionVtl         = 0x000D,
    HvCallDisablePartitionVtl        = 0x000E,
    HvCallEnableVpVtl                = 0x000F,
    HvCallDisableVpVtl               = 0x0010,
    HvCallVtlCall                    = 0x0011,
    HvCallVtlReturn                  = 0x0012,
    HvCallFlushVirtualAddressSpaceEx = 0x0013,
    HvCallFlushVirtualAddressListEx  = 0x0014,
    HvCallSendSyntheticClusterIpiEx  = 0x0015,
    // 0x0016..0x003F are reserved
    HvCreatePartition         = 0x0040,
    HvInitializePartition     = 0x0041,
    HvFinalizePartition       = 0x0042,
    HvDeletePartition         = 0x0043,
    HvGetPartitionProperty    = 0x0044,
    HvSetPartitionProperty    = 0x0045,
    HvGetPartitionId          = 0x0046,
    HvGetNextChildPartition   = 0x0047,
    HvDepositMemory           = 0x0048,
    HvWithdrawMemory          = 0x0049,
    HvGetMemoryBalance        = 0x004A,
    HvMapGpaPages             = 0x004B,
    HvUnmapGpaPages           = 0x004C,
    HvInstallIntercept        = 0x004D,
    HvCreateVp                = 0x004E,
    HvDeleteVp                = 0x004F,
    HvGetVpRegisters          = 0x0050,
    HvSetVpRegisters          = 0x0051,
    HvTranslateVirtualAddress = 0x0052,
    HvReadGpa                 = 0x0053,
    HvWriteGpa                = 0x0054,
    // 0x0055 is deprecated
    HvClearVirtualInterrupt = 0x0056,
    // 0x0057 is deprecated
    HvDeletePort                    = 0x0058,
    HvConnectPort                   = 0x0059,
    HvGetPortProperty               = 0x005A,
    HvDisconnectPort                = 0x005B,
    HvPostMessage                   = 0x005C,
    HvSignalEvent                   = 0x005D,
    HvSavePartitionState            = 0x005E,
    HvRestorePartitionState         = 0x005F,
    HvInitializeEventLogBufferGroup = 0x0060,
    HvFinalizeEventLogBufferGroup   = 0x0061,
    HvCreateEventLogBuffer          = 0x0062,
    HvDeleteEventLogBuffer          = 0x0063,
    HvMapEventLogBuffer             = 0x0064,
    HvUnmapEventLogBuffer           = 0x0065,
    HvSetEventLogGroupSources       = 0x0066,
    HvReleaseEventLogBuffer         = 0x0067,
    HvFlushEventLogBuffer           = 0x0068,
    HvPostDebugData                 = 0x0069,
    HvRetrieveDebugData             = 0x006A,
    HvResetDebugSession             = 0x006B,
    HvMapStatsPage                  = 0x006C,
    HvUnmapStatsPage                = 0x006D,
    HvCallMapSparseGpaPages         = 0x006E,
    HvCallSetSystemProperty         = 0x006F,
    HvCallSetPortProperty           = 0x0070,
    // 0x0071..0x0075 are reserved
    HvCallAddLogicalProcessor         = 0x0076,
    HvCallRemoveLogicalProcessor      = 0x0077,
    HvCallQueryNumaDistance           = 0x0078,
    HvCallSetLogicalProcessorProperty = 0x0079,
    HvCallGetLogicalProcessorProperty = 0x007A,
    HvCallGetSystemProperty           = 0x007B,
    HvCallMapDeviceInterrupt          = 0x007C,
    HvCallUnmapDeviceInterrupt        = 0x007D,
    HvCallRetargetDeviceInterrupt     = 0x007E,
    // 0x007F is reserved
    HvCallMapDevicePages               = 0x0080,
    HvCallUnmapDevicePages             = 0x0081,
    HvCallAttachDevice                 = 0x0082,
    HvCallDetachDevice                 = 0x0083,
    HvCallNotifyStandbyTransition      = 0x0084,
    HvCallPrepareForSleep              = 0x0085,
    HvCallPrepareForHibernate          = 0x0086,
    HvCallNotifyPartitionEvent         = 0x0087,
    HvCallGetLogicalProcessorRegisters = 0x0088,
    HvCallSetLogicalProcessorRegisters = 0x0089,
    HvCallQueryAssotiatedLpsforMca     = 0x008A,
    HvCallNotifyRingEmpty              = 0x008B,
    HvCallInjectSyntheticMachineCheck  = 0x008C,
    HvCallScrubPartition               = 0x008D,
    HvCallCollectLivedump              = 0x008E,
    HvCallDisableHypervisor            = 0x008F,
    HvCallModifySparseGpaPages         = 0x0090,
    HvCallRegisterInterceptResult      = 0x0091,
    HvCallUnregisterInterceptResult    = 0x0092,
    HvCallAssertVirtualInterrupt       = 0x0094,
    HvCallCreatePort                   = 0x0095,
    HvCallConnectPort                  = 0x0096,
    HvCallGetSpaPageList               = 0x0097,
    // 0x0098 is reserved
    HvCallStartVirtualProcessor = 0x009A,
    HvCallGetVpIndexFromApicId  = 0x009A,
    // 0x009A..0x00AE are reserved
    HvCallFlushGuestPhysicalAddressSpace = 0x00AF,
    HvCallFlushGuestPhysicalAddressList  = 0x00B0
};

//////////////////////////////////////////////////
//					Enums						//
//////////////////////////////////////////////////

/**
 * Guest IA32_DEBUGCTL.
 */
#define VMCS_GUEST_DEBUGCTL_HIGH 0x00002803
#define VIRTUAL_PROCESSOR_ID     0x00000000

/**
 * @brief MOV to debug registers states
 * 
 */
typedef enum MOV_TO_DEBUG_REG
{
    AccessToDebugRegister   = 0,
    AccessFromDebugRegister = 1,
};

//////////////////////////////////////////////////
//			 Structures & Unions				//
//////////////////////////////////////////////////

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
 * @brief The status of each core after and before VMX
 * 
 */
typedef struct _VIRTUAL_MACHINE_STATE
{
    BOOLEAN IsOnVmxRootMode;                                               // Detects whether the current logical core is on Executing on VMX Root Mode
    BOOLEAN IncrementRip;                                                  // Checks whether it has to redo the previous instruction or not (it used mainly in Ept routines)
    BOOLEAN HasLaunched;                                                   // Indicate whether the core is virtualized or not
    BOOLEAN IgnoreMtfUnset;                                                // Indicate whether the core should ignore unsetting the MTF or not
    BOOLEAN WaitForImmediateVmexit;                                        // Whether the current core is waiting for an immediate vm-exit or not
    PKDPC   KdDpcObject;                                                   // DPC object to be used in kernel debugger
    UINT64  LastVmexitRip;                                                 // RIP in the current VM-exit
    UINT64  VmxonRegionPhysicalAddress;                                    // Vmxon region physical address
    UINT64  VmxonRegionVirtualAddress;                                     // VMXON region virtual address
    UINT64  VmcsRegionPhysicalAddress;                                     // VMCS region physical address
    UINT64  VmcsRegionVirtualAddress;                                      // VMCS region virtual address
    UINT64  VmmStack;                                                      // Stack for VMM in VM-Exit State
    UINT64  MsrBitmapVirtualAddress;                                       // Msr Bitmap Virtual Address
    UINT64  MsrBitmapPhysicalAddress;                                      // Msr Bitmap Physical Address
    UINT64  IoBitmapVirtualAddressA;                                       // I/O Bitmap Virtual Address (A)
    UINT64  IoBitmapPhysicalAddressA;                                      // I/O Bitmap Physical Address (A)
    UINT64  IoBitmapVirtualAddressB;                                       // I/O Bitmap Virtual Address (B)
    UINT64  IoBitmapPhysicalAddressB;                                      // I/O Bitmap Physical Address (B)
    UINT32  PendingExternalInterrupts[PENDING_INTERRUPTS_BUFFER_CAPACITY]; // This list holds a buffer for external-interrupts that are in pending state due to the external-interrupt
                                                                           // blocking and waits for interrupt-window exiting
                                                                           // From hvpp :
                                                                           // Pending interrupt queue (FIFO).
                                                                           // Make storage for up-to 64 pending interrupts.
                                                                           // In practice I haven't seen more than 2 pending interrupts.

    PROCESSOR_DEBUGGING_STATE DebuggingState;         // Holds the debugging state of the processor (used by HyperDbg to execute commands)
    VMX_VMXOFF_STATE          VmxoffState;            // Shows the vmxoff state of the guest
    VM_EXIT_TRANSPARENCY      TransparencyState;      // The state of the debugger in transparent-mode
    PEPT_HOOKED_PAGE_DETAIL   MtfEptHookRestorePoint; // It shows the detail of the hooked paged that should be restore in MTF vm-exit
    MEMORY_MAPPER_ADDRESSES   MemoryMapper;           // Memory mapper details for each core, contains PTE Virtual Address, Actual Kernel Virtual Address
} VIRTUAL_MACHINE_STATE, *PVIRTUAL_MACHINE_STATE;

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

BOOLEAN
VmxCheckVmxSupport();

BOOLEAN
VmxInitialize();

BOOLEAN
VmxPerformVirtualizationOnAllCores();

BOOLEAN
VmxTerminate();

VOID
VmxPerformTermination();

_Success_(return != FALSE)
BOOLEAN
VmxAllocateVmxonRegion(_Out_ VIRTUAL_MACHINE_STATE * CurrentGuestState);

_Success_(return != FALSE)
BOOLEAN
VmxAllocateVmcsRegion(_Out_ VIRTUAL_MACHINE_STATE * CurrentGuestState);

BOOLEAN
VmxAllocateVmmStack(_In_ INT ProcessorID);

BOOLEAN
VmxAllocateMsrBitmap(_In_ INT ProcessorID);

BOOLEAN
VmxAllocateIoBitmaps(_In_ INT ProcessorID);

VOID
VmxHandleXsetbv(UINT32 Reg, UINT64 Value);

VOID
VmxHandleVmxPreemptionTimerVmexit(UINT32 CurrentCoreIndex, PGUEST_REGS GuestRegs);

VOID
VmxVmptrst();

VOID
VmxVmresume();

VOID
VmxVmxoff();

BOOLEAN
VmxPerformVirtualizationOnSpecificCore();

VOID
VmxFixCr4AndCr0Bits();

BOOLEAN
VmxLoadVmcs(_In_ VIRTUAL_MACHINE_STATE * CurrentGuestState);

BOOLEAN
VmxClearVmcsState(_In_ VIRTUAL_MACHINE_STATE * CurrentGuestState);

BOOLEAN
VmxCheckIsOnVmxRoot();

BOOLEAN
VmxVirtualizeCurrentSystem(PVOID GuestStack);

BOOLEAN
VmxSetupVmcs(_In_ VIRTUAL_MACHINE_STATE * CurrentGuestState, _In_ PVOID GuestStack);

UINT64
VmxReturnStackPointerForVmxoff();

UINT64
VmxReturnInstructionPointerForVmxoff();
