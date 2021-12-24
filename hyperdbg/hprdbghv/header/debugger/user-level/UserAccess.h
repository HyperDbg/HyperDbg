/**
 * @file UserAccess.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Access and parse user-mode components of binaries
 * @details Access to Portable Executables
 * @version 0.1
 * @date 2021-12-24
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//				   Definitions					//
//////////////////////////////////////////////////

typedef NTSTATUS (*QUERY_INFO_PROCESS)(
    __in HANDLE                                  ProcessHandle,
    __in PROCESSINFOCLASS                        ProcessInformationClass,
    __out_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG                                   ProcessInformationLength,
    __out_opt PULONG                             ReturnLength);

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

BOOLEAN
UserAccessAllocateAndGetImagePathFromProcessId(HANDLE          ProcessId,
                                               PUNICODE_STRING ProcessImageName,
                                               UINT32          SizeOfImageNameToBeAllocated);
BOOLEAN
UserAccessGetPebFromProcessId(HANDLE ProcessId, PUINT64 Peb);
