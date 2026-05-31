/**
 * @file Lbr.h
 * @author Hari Mishal (harimishal6@gmail.com)
 * @brief Message logging and tracing implementation
 * @details Modified from LIBIHT project (Thomasaon Zhao et al) with Windows style updates.
 * @version 0.18
 * @date 2025-12-02
 *
 * @copyright This project is released under the GNU Public License v3.
 */
#pragma once

//////////////////////////////////////////////////
//			    	  Constants	    			//
//////////////////////////////////////////////////

//
// Legacy LBR MSRs
//
#ifndef MSR_LEGACY_LBR_SELECT
#    define MSR_LEGACY_LBR_SELECT 0x000001C8 // originally defined in SDK to be used by other module like HyperHV
#endif                                       // !MSR_LEGACY_LBR_SELECT
#define MSR_LBR_TOS               0x000001C9
#define MSR_LASTBRANCH_0_FROM_IP  0x00000680
#define MSR_LASTBRANCH_0_TO_IP    0x000006C0
#define MSR_LASTBRANCH_INFO_0     0x00000DC0
#define LBR_SELECT_WITHOUT_FILTER 0x00000000

//
// Arch LBR MSRs
//
#define IA32_LBR_0_FROM_IP 0x1500
#define IA32_LBR_0_TO_IP   0x1600
#define IA32_LBR_0_INFO    0x1200

#define CPUID_ARCH_LAST_BRANCH_RECORD_INFORMATION 0x1c

//
// This MSR could be used as an alternative to MSR_LBR_SELECT and IA32_DEBUGCTL for enabling and configuring LBR
// For using that in hypervisor Load Guest IA32_LBR_CTL Entry Control and Clear IA32_LBR_CTL Exit Control should
// be configured, plus host could control it over Guest IA32_LBR_CTL on VMCS
//
#define IA32_LBR_CTL 0x000014CE

//////////////////////////////////////////////////
//               CPUID Structures               //
//////////////////////////////////////////////////

/*
 * @brief Intel Architectural LBR CPUID detection/enumeration details:
 */
typedef union _CPUID28_EAX
{
    struct
    {
        /* Supported LBR depth values */
        UINT32 LbrDepthMask : 8;
        UINT32 Reserved : 22;
        /* Deep C-state Reset */
        UINT32 LbrDeepCReset : 1;
        /* IP values contain LIP */
        UINT32 LbrLip : 1;
    };
    UINT32 AsUInt;
} CPUID28_EAX, *PCPUID28_EAX;

typedef union _CPUID28_EBX
{
    struct
    {
        /* CPL Filtering Supported */
        UINT32 LbrCpl : 1;
        /* Branch Filtering Supported */
        UINT32 LbrFilter : 1;
        /* Call-stack Mode Supported */
        UINT32 LbrCallStack : 1;
        UINT32 Reserved : 29;
    };
    UINT32 AsUInt;
} CPUID28_EBX, *PCPUID28_EBX;

typedef union _CPUID28_ECX
{
    struct
    {
        /* Mispredict Bit Supported */
        UINT32 LbrMispred : 1;
        /* Timed LBRs Supported */
        UINT32 LbrTimedLbr : 1;
        /* Branch Type Field Supported */
        UINT32 LbrBrType : 1;
        UINT32 Reserved : 29;
    };
    UINT32 AsUInt;
} CPUID28_ECX, *PCPUID28_ECX;

/*
 * @brief The structure to hold the CPUID leaf 0x28 details for Architectural LBRs
 */
typedef struct _CPUID28_LEAFS
{
    CPUID28_EAX Eax;
    CPUID28_EBX Ebx;
    CPUID28_ECX Ecx;
    UINT32      Edx;
} CPUID28_LEAFS, *PCPUID28_LEAFS;

//////////////////////////////////////////////////
//               MSR Structures                 //
//////////////////////////////////////////////////

/**
 * @brief The structure to hold the IA32_LBR_CTL MSR, which is used to enable and configure the LBR feature
 * @details MSR Address: 0x14CEH (Hex) / 5326 (Dec)
 */
typedef union _IA32_LBR_CTL_REGISTER
{
    ULONG64 AsUInt;

    struct
    {
        ULONG64 LBREn : 1;       // [0]     When set, enables LBR recording
        ULONG64 OS : 1;          // [1]     When set, allows LBR recording when CPL == 0
        ULONG64 USR : 1;         // [2]     When set, allows LBR recording when CPL != 0
        ULONG64 CallStack : 1;   // [3]     When set, records branches in call-stack mode (See Section 7.1.2.4)
        ULONG64 Reserved0 : 12;  // [15:4]  Reserved (must be zero)
        ULONG64 JCC : 1;         // [16]    When set, records taken conditional branches (See Section 7.1.2.3)
        ULONG64 NearRelJmp : 1;  // [17]    When set, records near relative JMPs (See Section 7.1.2.3)
        ULONG64 NearIndJmp : 1;  // [18]    When set, records near indirect JMPs (See Section 7.1.2.3)
        ULONG64 NearRelCall : 1; // [19]    When set, records near relative CALLs (See Section 7.1.2.3)
        ULONG64 NearIndCall : 1; // [20]    When set, records near indirect CALLs (See Section 7.1.2.3)
        ULONG64 NearRet : 1;     // [21]    When set, records near RETs (See Section 7.1.2.3)
        ULONG64 OtherBranch : 1; // [22]    When set, records other branches (See Section 7.1.2.3)
        ULONG64 Reserved1 : 41;  // [63:23] Reserved (must be zero)
    } Bits;

} IA32_LBR_CTL_REGISTER, *PIA32_LBR_CTL_REGISTER;

//////////////////////////////////////////////////
//                Global Variables              //
//////////////////////////////////////////////////

/**
 * @brief The structure to hold the mapping of CPU model to its LBR capacity
 *
 */
typedef struct _CPU_LBR_MAP
{
    ULONG Model;
    ULONG LbrCapacity;
} CPU_LBR_MAP, *PCPU_LBR_MAP;

/**
 * @brief The global variable to hold the mapping of CPU model to its LBR capacity
 *
 */
extern CPU_LBR_MAP CPU_LBR_MAPS[];

//////////////////////////////////////////////////
//                  Functions                   //
//////////////////////////////////////////////////

BOOLEAN
LbrCheckAndReadLegacyLbrDetails();

BOOLEAN
LbrCheckAndReadArchitecturalLbrDetails();

BOOLEAN
LbrStart(UINT64 FilterOptions);

BOOLEAN
LbrCheck();

VOID
LbrFilter(UINT64 FilterOptions);

VOID
LbrStop();

VOID
LbrFlush();

VOID
LbrSave();

VOID
LbrPrint();
