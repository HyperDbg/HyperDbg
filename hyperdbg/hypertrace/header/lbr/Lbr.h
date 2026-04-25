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

#define MSR_LBR_SELECT   0x000001C8
#define MSR_LBR_TOS      0x000001C9
#define MSR_LBR_NHM_FROM 0x00000680
#define MSR_LBR_NHM_TO   0x000006C0
#define LBR_SELECT       0x00000000

/**
 * @brief Maximum LBR capacity that is supported by processors
 *
 */
#define MAXIMUM_LBR_CAPACITY 0x20 // 32 entries, which is the maximum supported by modern Intel CPUs

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
    UINT32           Tos;

} LBR_STACK_ENTRY, PLBR_STACK_ENTRY;

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

/**
 * @brief The global variable to hold the LBR capacity of the current CPU
 *
 */
ULONGLONG LbrCapacity;

//////////////////////////////////////////////////
//                  Functions                   //
//////////////////////////////////////////////////

BOOLEAN
LbrCheck();

BOOLEAN
LbrStart(BOOLEAN ApplyFromVmxRootMode, BOOLEAN ApplyByVmcall);

VOID
LbrStop(BOOLEAN ApplyFromVmxRootMode, BOOLEAN ApplyByVmcall);

VOID
LbrSave();

VOID
LbrDump();

extern ULONGLONG  LbrCapacity;
extern LIST_ENTRY LbrStateHead;
extern KSPIN_LOCK LbrStateLock;
