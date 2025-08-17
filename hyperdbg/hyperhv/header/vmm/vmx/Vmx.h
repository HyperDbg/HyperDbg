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

#define IS_VALID_DEBUG_REGISTER(DebugRegister) \
    (((DebugRegister <= VMX_EXIT_QUALIFICATION_REGISTER_DR0) && (DebugRegister <= VMX_EXIT_QUALIFICATION_REGISTER_DR7)) && (DebugRegister != 0x00000004 && DebugRegister != 0x00000005))

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
typedef enum _MOV_TO_DEBUG_REG
{
    AccessToDebugRegister   = 0,
    AccessFromDebugRegister = 1,
} MOV_TO_DEBUG_REG;

//////////////////////////////////////////////////
//				VMX Instructions				//
//////////////////////////////////////////////////

extern inline UCHAR
VmxVmread64(size_t Field, UINT64 FieldValue);

extern inline UCHAR
VmxVmread32(size_t Field, UINT32 FieldValue);

extern inline UCHAR
VmxVmread16(size_t Field, UINT16 FieldValue);

extern inline UCHAR
VmxVmread64P(size_t Field, UINT64 * FieldValue);

extern inline UCHAR
VmxVmread32P(size_t Field, UINT32 * FieldValue);

extern inline UCHAR
VmxVmread16P(size_t Field, UINT16 * FieldValue);

extern inline UCHAR
VmxVmwrite64(size_t Field, UINT64 FieldValue);

extern inline UCHAR
VmxVmwrite32(size_t Field, UINT32 FieldValue);

extern inline UCHAR
VmxVmwrite16(size_t Field, UINT16 FieldValue);

VOID
VmxVmptrst();

VOID
VmxVmresume();

VOID
VmxVmxoff(VIRTUAL_MACHINE_STATE * VCpu);

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

VOID
VmxHandleXsetbv(VIRTUAL_MACHINE_STATE * VCpu);

VOID
VmxHandleVmxPreemptionTimerVmexit(VIRTUAL_MACHINE_STATE * VCpu);

VOID
VmxHandleTripleFaults(VIRTUAL_MACHINE_STATE * VCpu);

BOOLEAN
VmxPerformVirtualizationOnSpecificCore();

VOID
VmxFixCr4AndCr0Bits();

BOOLEAN
VmxLoadVmcs(_In_ VIRTUAL_MACHINE_STATE * VCpu);

BOOLEAN
VmxClearVmcsState(_In_ VIRTUAL_MACHINE_STATE * VCpu);

BOOLEAN
VmxCheckIsOnVmxRoot();

BOOLEAN
VmxVirtualizeCurrentSystem(PVOID GuestStack);

BOOLEAN
VmxSetupVmcs(_In_ VIRTUAL_MACHINE_STATE * VCpu, _In_ PVOID GuestStack);

UINT64
VmxReturnStackPointerForVmxoff();

UINT64
VmxReturnInstructionPointerForVmxoff();

BOOLEAN
VmxGetCurrentExecutionMode();

BOOLEAN
VmxGetCurrentLaunchState();

UINT32
VmxCompatibleStrlen(const CHAR * S);

UINT32
VmxCompatibleWcslen(const wchar_t * S);

INT32
VmxCompatibleStrcmp(const CHAR * Address1,
                    const CHAR * Address2,
                    SIZE_T       Num,
                    BOOLEAN      IsStrncmp);

INT32
VmxCompatibleWcscmp(const wchar_t * Address1,
                    const wchar_t * Address2,
                    SIZE_T          Num,
                    BOOLEAN         IsWcsncmp);

INT32
VmxCompatibleMemcmp(const CHAR * Address1,
                    const CHAR * Address2,
                    size_t       Count);

VOID
VmxCompatibleMicroSleep(UINT64 ns);
