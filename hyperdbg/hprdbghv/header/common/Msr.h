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
