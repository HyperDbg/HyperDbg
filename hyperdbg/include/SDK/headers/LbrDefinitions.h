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
#define MSR_LBR_SELECT 0x000001C8

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
#define LBR_KERNEL     (1 << LBR_KERNEL_BIT)
#define LBR_USER       (1 << LBR_USER_BIT)
#define LBR_JCC        (1 << LBR_JCC_BIT)
#define LBR_REL_CALL   (1 << LBR_REL_CALL_BIT)
#define LBR_IND_CALL   (1 << LBR_IND_CALL_BIT)
#define LBR_RETURN     (1 << LBR_RETURN_BIT)
#define LBR_IND_JMP    (1 << LBR_IND_JMP_BIT)
#define LBR_REL_JMP    (1 << LBR_REL_JMP_BIT)
#define LBR_FAR        (1 << LBR_FAR_BIT)
#define LBR_CALL_STACK (1 << LBR_CALL_STACK_BIT)

/**
 * @brief Maximum LBR capacity that is supported by processors
 *
 */
#define MAXIMUM_LBR_CAPACITY 0x20 // 32 entries, which is the maximum supported by modern Intel CPUs
