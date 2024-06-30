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
//				External Functions				//
//////////////////////////////////////////////////

UCHAR *
PsGetProcessImageFileName(IN PEPROCESS Process);

PVOID
PsGetProcessSectionBaseAddress(PEPROCESS Process); // Used to get the base address of process's executable image

NTKERNELAPI NTSTATUS NTAPI
SeCreateAccessState(
    PACCESS_STATE    AccessState,
    PVOID            AuxData,
    ACCESS_MASK      DesiredAccess,
    PGENERIC_MAPPING Mapping);

NTKERNELAPI VOID NTAPI
SeDeleteAccessState(
    PACCESS_STATE AccessState);

NTSTATUS
MmUnmapViewOfSection(PEPROCESS Process, PVOID BaseAddress); // Used to unmap process's executable image

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
//		        	  Enums 			    	//
//////////////////////////////////////////////////

/**
 * @brief Different methods of killing a process
 *
 */
typedef enum _PROCESS_KILL_METHODS
{
    PROCESS_KILL_METHOD_1 = 0,
    PROCESS_KILL_METHOD_2,
    PROCESS_KILL_METHOD_3,

} PROCESS_KILL_METHODS;

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
CommonIsProcessExist(UINT32 ProcId);

PCHAR
CommonGetProcessNameFromProcessControlBlock(PEPROCESS Eprocess);

BOOLEAN
CommonKillProcess(UINT32 ProcessId, PROCESS_KILL_METHODS KillingMethod);

BOOLEAN
CommonValidateCoreNumber(UINT32 CoreNumber);
