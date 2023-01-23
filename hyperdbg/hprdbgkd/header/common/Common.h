/**
 * @file Common.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Routines for common tasks in debugger
 * @details
 *
 * @version 0.2
 * @date 2023-01-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//         Windows-specific structures          //
//////////////////////////////////////////////////

/**
 * @brief KPROCESS Brief structure
 *
 */
typedef struct _NT_KPROCESS
{
    DISPATCHER_HEADER Header;
    LIST_ENTRY        ProfileListHead;
    ULONG_PTR         DirectoryTableBase;
    UCHAR             Data[1];
} NT_KPROCESS, *PNT_KPROCESS;

//////////////////////////////////////////////////
//		        	  Constants			    	//
//////////////////////////////////////////////////

/*
 * @brief Segment register and corresponding GDT meaning in Windows
 */
#define KGDT64_NULL      (0 * 16)     // NULL descriptor
#define KGDT64_R0_CODE   (1 * 16)     // kernel mode 64-bit code
#define KGDT64_R0_DATA   (1 * 16) + 8 // kernel mode 64-bit data (stack)
#define KGDT64_R3_CMCODE (2 * 16)     // user mode 32-bit code
#define KGDT64_R3_DATA   (2 * 16) + 8 // user mode 32-bit data
#define KGDT64_R3_CODE   (3 * 16)     // user mode 64-bit code
#define KGDT64_SYS_TSS   (4 * 16)     // kernel mode system task state
#define KGDT64_R3_CMTEB  (5 * 16)     // user mode 32-bit TEB
#define KGDT64_R0_CMCODE (6 * 16)     // kernel mode 32-bit code
#define KGDT64_LAST      (7 * 16)     // last entry

//////////////////////////////////////////////////
//		    	 Loader Functions				//
//////////////////////////////////////////////////

BOOLEAN
IsProcessExist(UINT32 ProcId);

PCHAR
GetProcessNameFromEprocess(PEPROCESS Eprocess);
