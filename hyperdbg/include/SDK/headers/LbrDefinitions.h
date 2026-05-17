/**
 * @file LbrDefinitions.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Last Branch Record (LBR) related data structures
 * @details
 * @version 0.19
 * @date 2026-04-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//		            Constants                   //
//////////////////////////////////////////////////

/**
 * @brief MSR address of LBR_SELECT, which is used to configure the LBR filtering options
 */
#define MSR_LEGACY_LBR_SELECT 0x000001C8

/*
 * Intel LBR_SELECT bits
 *
 * Hardware branch filter (not available on all CPUs)
 */
#define LBR_KERNEL_BIT     0 /* do not capture at ring0 */
#define LBR_USER_BIT       1 /* do not capture at ring > 0 */
#define LBR_JCC_BIT        2 /* do not capture conditional branches */
#define LBR_REL_CALL_BIT   3 /* do not capture relative calls */
#define LBR_IND_CALL_BIT   4 /* do not capture indirect calls */
#define LBR_RETURN_BIT     5 /* do not capture near returns */
#define LBR_IND_JMP_BIT    6 /* do not capture indirect jumps */
#define LBR_REL_JMP_BIT    7 /* do not capture relative jumps */
#define LBR_FAR_BIT        8 /* do not capture far branches */
#define LBR_CALL_STACK_BIT 9 /* enable call stack: not available on all CPUs */

/*
 * We mask it out before writing it to
 * the actual MSR. But it helps the constraint code to understand
 * that this is a separate configuration.
 */
#define LBR_KERNEL             (1 << LBR_KERNEL_BIT)
#define LBR_USER               (1 << LBR_USER_BIT)
#define LBR_JCC                (1 << LBR_JCC_BIT)
#define LBR_REL_CALL           (1 << LBR_REL_CALL_BIT)
#define LBR_IND_CALL           (1 << LBR_IND_CALL_BIT)
#define LBR_RETURN             (1 << LBR_RETURN_BIT)
#define LBR_IND_JMP            (1 << LBR_IND_JMP_BIT)
#define LBR_REL_JMP            (1 << LBR_REL_JMP_BIT)
#define LBR_FAR_OTHER_BRANCHES (1 << LBR_FAR_BIT) // It is used for OTHER BRANCHES in ARCH LBR
#define LBR_CALL_STACK         (1 << LBR_CALL_STACK_BIT)

/**
 * @brief Maximum LBR capacity that is supported by processors
 *
 */
#define MAXIMUM_LBR_CAPACITY 0x20 // 32 entries, which is the maximum supported by modern Intel CPUs

/**
 * @brief Branch Type Encodings (Only on Architectural LBR, not available in Legacy LBR)
 */
#define LBR_BR_TYPE_COND          0x0
#define LBR_BR_TYPE_JMP_INDIRECT  0x1
#define LBR_BR_TYPE_JMP_DIRECT    0x2
#define LBR_BR_TYPE_CALL_INDIRECT 0x3
#define LBR_BR_TYPE_CALL_DIRECT   0x4
#define LBR_BR_TYPE_RET           0x5
#define LBR_BR_TYPE_RESERVED_MIN  0x6 /* 011xb */
#define LBR_BR_TYPE_RESERVED_MAX  0x7 /* 011xb */
#define LBR_BR_TYPE_OTHER_MIN     0x8 /* 1xxxb */
#define LBR_BR_TYPE_OTHER_MAX     0xF /* 1xxxb */

#define LBR_BR_TYPE_NAME_MAX_LEN 16 /* longest string is "CALL Indirect\0" = 14 chars, rounded up */

//////////////////////////////////////////////////
//               MSR Structures                 //
//////////////////////////////////////////////////
/**
 * MSR_LBR_INFO_x - Last Branch Record Info Register
 * Register Address: 1200H-121FH, 4608-4639
 */
typedef union
{
    struct
    {
        /** Bits 15:0 - Elapsed core clocks since last update to the LBR stack (saturating) */
        UINT64 CycleCount : 16;

        /** Bits 55:16 - Undefined. May be zero or non-zero.
         *  Writes of non-zero values do not fault, but reads may return a different value. */
        UINT64 Reserved : 40;

        /**
         * Bits 59:56 - Branch type recorded by this LBR entry.
         * Encodings:
         *   0000b = COND
         *   0001b = JMP Indirect
         *   0010b = JMP Direct
         *   0011b = CALL Indirect
         *   0100b = CALL Direct
         *   0101b = RET
         *   011xb = Reserved
         *   1xxxb = Other Branch
         */
        UINT64 BrType_OnlyArchLbr : 4;

        /** Bit 60 - When set, the CycleCount field contains a valid elapsed cycle count */
        UINT64 CycCntValid_OnlyArchLbr : 1;

        /**
         * Bit 61 - TSX Abort indicator.
         * When set:
         *   LBR_FROM = EIP at the time of the TSX Abort
         *   LBR_TO   = EIP of the start of HLE region OR EIP of the RTM Abort Handler
         * Undefined on processors that do not support Intel TSX
         * (CPUID.07H.00H:EBX.HLE[4] = 0 and CPUID.07H.00H:EBX.RTM[11] = 0).
         */
        UINT64 TsxAbort : 1;

        /**
         * Bit 62 - When set, indicates the branch retired during a TSX transaction.
         * Undefined on processors that do not support Intel TSX
         * (CPUID.07H.00H:EBX.HLE[4] = 0 and CPUID.07H.00H:EBX.RTM[11] = 0).
         */
        UINT64 InTsx : 1;

        /**
         * Bit 63 - Branch misprediction flag.
         * When set, the target of the branch was mispredicted and/or the
         * direction (taken/non-taken) was mispredicted.
         * When clear, the branch target/direction was correctly predicted.
         */
        UINT64 Mispred : 1;
    };
    UINT64 AsUInt;
} MSR_LBR_INFO, *PMSR_LBR_INFO;

//////////////////////////////////////////////////
//                  Structures                  //
//////////////////////////////////////////////////

/**
 * @brief The structure to hold a single LBR entry (from and to addresses)
 *
 */
typedef struct _LBR_BRANCH_ENTRY
{
    ULONGLONG From;
    ULONGLONG To;

} LBR_BRANCH_ENTRY, PLBR_BRANCH_ENTRY;

/**
 * @brief The structure to hold the LBR stack for a single processor core, including the branch entries and the TOS index
 *
 */
typedef struct _LBR_STACK_ENTRY
{
    LBR_BRANCH_ENTRY BranchEntry[MAXIMUM_LBR_CAPACITY];
    MSR_LBR_INFO     LastBranchInfo[MAXIMUM_LBR_CAPACITY];
    UINT8            Tos;

} LBR_STACK_ENTRY, PLBR_STACK_ENTRY;
