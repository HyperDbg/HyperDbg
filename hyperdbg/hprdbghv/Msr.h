/**
 * @file Msr.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Model-Specific Registers Numbers and definitions
 * @details This file does not contain all the MSRs, instead it contains general MSRs
 * that used in this projec, some other MSRs e.g., EPT related MSRs are in their specific
 * headers
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//				    Constants					//
//////////////////////////////////////////////////

#define MSR_APIC_BASE                    0x0000001B
#define MSR_IA32_FEATURE_CONTROL         0x0000003A
#define MSR_IA32_VMX_BASIC               0x00000480
#define MSR_IA32_VMX_PINBASED_CTLS       0x00000481
#define MSR_IA32_VMX_PROCBASED_CTLS      0x00000482
#define MSR_IA32_VMX_EXIT_CTLS           0x00000483
#define MSR_IA32_VMX_ENTRY_CTLS          0x00000484
#define MSR_IA32_VMX_MISC                0x00000485
#define MSR_IA32_VMX_CR0_FIXED0          0x00000486
#define MSR_IA32_VMX_CR0_FIXED1          0x00000487
#define MSR_IA32_VMX_CR4_FIXED0          0x00000488
#define MSR_IA32_VMX_CR4_FIXED1          0x00000489
#define MSR_IA32_VMX_VMCS_ENUM           0x0000048A
#define MSR_IA32_VMX_PROCBASED_CTLS2     0x0000048B
#define MSR_IA32_VMX_EPT_VPID_CAP        0x0000048C
#define MSR_IA32_VMX_TRUE_PINBASED_CTLS  0x0000048D
#define MSR_IA32_VMX_TRUE_PROCBASED_CTLS 0x0000048E
#define MSR_IA32_VMX_TRUE_EXIT_CTLS      0x0000048F
#define MSR_IA32_VMX_TRUE_ENTRY_CTLS     0x00000490
#define MSR_IA32_VMX_VMFUNC              0x00000491
#define MSR_IA32_SYSENTER_CS             0x00000174
#define MSR_IA32_SYSENTER_ESP            0x00000175
#define MSR_IA32_SYSENTER_EIP            0x00000176
#define MSR_IA32_DEBUGCTL                0x000001D9
#define MSR_EFER                         0xC0000080
#define MSR_STAR                         0xC0000081
#define MSR_LSTAR                        0xC0000082
#define MSR_FMASK                        0xC0000084
#define MSR_FS_BASE                      0xC0000100
#define MSR_GS_BASE                      0xC0000101
#define MSR_SHADOW_GS_BASE               0xC0000102

//////////////////////////////////////////////////
//				    Structures					//
//////////////////////////////////////////////////

/**
 * @brief EFER MSR Structure
 * 
 */
typedef union _EFER_MSR
{
    struct
    {
        UINT64 SyscallEnable : 1;
        UINT64 Reserved1 : 7;
        UINT64 Ia32eModeEnable : 1;
        UINT64 Reserved2 : 1;
        UINT64 Ia32eModeActive : 1;
        UINT64 ExecuteDisableBitEnable : 1;
        UINT64 Reserved3 : 52;
    };
    UINT64 Flags;
} EFER_MSR, *PEFER_MSR;

/**
 * @brief IA32_FEATURE_CONTROL_MSR Structure
 * 
 */
typedef union _IA32_FEATURE_CONTROL_MSR
{
    ULONG64 All;
    struct
    {
        ULONG64 Lock : 1;               // [0]
        ULONG64 EnableSMX : 1;          // [1]
        ULONG64 EnableVmxon : 1;        // [2]
        ULONG64 Reserved2 : 5;          // [3-7]
        ULONG64 EnableLocalSENTER : 7;  // [8-14]
        ULONG64 EnableGlobalSENTER : 1; // [15]
        ULONG64 Reserved3a : 16;        //
        ULONG64 Reserved3b : 32;        // [16-63]
    } Fields;
} IA32_FEATURE_CONTROL_MSR, *PIA32_FEATURE_CONTROL_MSR;

/**
 * @brief VMX Basic Information MSR Structure
 * 
 */
typedef union _IA32_VMX_BASIC_MSR
{
    ULONG64 All;
    struct
    {
        ULONG32 RevisionIdentifier : 31;  // [0-30]
        ULONG32 Reserved1 : 1;            // [31]
        ULONG32 RegionSize : 12;          // [32-43]
        ULONG32 RegionClear : 1;          // [44]
        ULONG32 Reserved2 : 3;            // [45-47]
        ULONG32 SupportedIA64 : 1;        // [48]
        ULONG32 SupportedDualMoniter : 1; // [49]
        ULONG32 MemoryType : 4;           // [50-53]
        ULONG32 VmExitReport : 1;         // [54]
        ULONG32 VmxCapabilityHint : 1;    // [55]
        ULONG32 Reserved3 : 8;            // [56-63]
    } Fields;
} IA32_VMX_BASIC_MSR, *PIA32_VMX_BASIC_MSR;

/**
 * @brief General MSR Structure
 * 
 */
typedef union _MSR
{
    struct
    {
        ULONG Low;
        ULONG High;
    };

    ULONG64 Content;
} MSR, *PMSR;
