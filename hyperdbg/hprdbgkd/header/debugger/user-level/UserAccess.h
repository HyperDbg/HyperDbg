/**
 * @file UserAccess.h
 * @author Sina Karvandi (sina@hyperdbg.org)
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
//				   Structures					//
//////////////////////////////////////////////////

/**
 * @brief PEB LDR Data
 * 
 */
typedef struct _PEB_LDR_DATA
{
    ULONG      Length;
    BOOLEAN    Initialized;
    PVOID      SsHandle;
    LIST_ENTRY ModuleListLoadOrder;
    LIST_ENTRY ModuleListMemoryOrder;
    LIST_ENTRY ModuleListInitOrder;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

/**
 * @brief User process params
 * 
 */
typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    BYTE           Reserved1[16];
    PVOID          Reserved2[10];
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

/**
 * @brief Random windows type
 * 
 */
typedef void(__stdcall * PPS_POST_PROCESS_INIT_ROUTINE)(void); // not exported

/**
 * @brief PEB 64-bit
 * 
 */
typedef struct _PEB
{
    BYTE                          Reserved1[2];
    BYTE                          BeingDebugged;
    BYTE                          Reserved2[1];
    PVOID                         Reserved3[2];
    PPEB_LDR_DATA                 Ldr;
    PRTL_USER_PROCESS_PARAMETERS  ProcessParameters;
    PVOID                         Reserved4[3];
    PVOID                         AtlThunkSListPtr;
    PVOID                         Reserved5;
    ULONG                         Reserved6;
    PVOID                         Reserved7;
    ULONG                         Reserved8;
    ULONG                         AtlThunkSListPtr32;
    PVOID                         Reserved9[45];
    BYTE                          Reserved10[96];
    PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
    BYTE                          Reserved11[128];
    PVOID                         Reserved12[1];
    ULONG                         SessionId;
} PEB, *PPEB;

/**
 * @brief PEB 32-bit
 * 
 */
typedef struct _PEB32
{
    UCHAR InheritedAddressSpace;
    UCHAR ReadImageFileExecOptions;
    UCHAR BeingDebugged;
    UCHAR BitField;
    ULONG Mutant;
    ULONG ImageBaseAddress;
    ULONG Ldr;
    ULONG ProcessParameters;
    ULONG SubSystemData;
    ULONG ProcessHeap;
    ULONG FastPebLock;
    ULONG AtlThunkSListPtr;
    ULONG IFEOKey;
    ULONG CrossProcessFlags;
    ULONG UserSharedInfoPtr;
    ULONG SystemReserved;
    ULONG AtlThunkSListPtr32;
    ULONG ApiSetMap;
} PEB32, *PPEB32;

/**
 * @brief LDR Data 32-bit
 * 
 */
typedef struct _PEB_LDR_DATA32
{
    ULONG        Length;
    UCHAR        Initialized;
    ULONG        SsHandle;
    LIST_ENTRY32 InLoadOrderModuleList;
    LIST_ENTRY32 InMemoryOrderModuleList;
    LIST_ENTRY32 InInitializationOrderModuleList;
} PEB_LDR_DATA32, *PPEB_LDR_DATA32;

/**
 * @brief LDR Data Table 32-bit
 * 
 */
typedef struct _LDR_DATA_TABLE_ENTRY32
{
    LIST_ENTRY32     InLoadOrderLinks;
    LIST_ENTRY32     InMemoryOrderLinks;
    LIST_ENTRY32     InInitializationOrderLinks;
    ULONG            DllBase;
    ULONG            EntryPoint;
    ULONG            SizeOfImage;
    UNICODE_STRING32 FullDllName;
    UNICODE_STRING32 BaseDllName;
    ULONG            Flags;
    UINT16           LoadCount;
    UINT16           TlsIndex;
    LIST_ENTRY32     HashLinks;
    ULONG            TimeDateStamp;
} LDR_DATA_TABLE_ENTRY32, *PLDR_DATA_TABLE_ENTRY32;

/**
 * @brief LDR Data Table 64-bit
 * 
 */
typedef struct _LDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY     InLoadOrderModuleList;
    LIST_ENTRY     InMemoryOrderModuleList;
    LIST_ENTRY     InInitializationOrderModuleList;
    PVOID          DllBase;
    PVOID          EntryPoint;
    ULONG          SizeOfImage; // in bytes
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG          Flags; // LDR_*
    UINT16         LoadCount;
    UINT16         TlsIndex;
    LIST_ENTRY     HashLinks;
    PVOID          SectionPointer;
    ULONG          CheckSum;
    ULONG          TimeDateStamp;
    //    PVOID			LoadedImports;
    //    seems they are exist only on XP
    //    EntryPointActivationContext;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

//////////////////////////////////////////////////
//				   Definitions					//
//////////////////////////////////////////////////

typedef NTSTATUS (*ZwQueryInformationProcess)(
    __in HANDLE                                  ProcessHandle,
    __in PROCESSINFOCLASS                        ProcessInformationClass,
    __out_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG                                   ProcessInformationLength,
    __out_opt PULONG                             ReturnLength);

typedef PPEB(NTAPI * PsGetProcessPeb)(PEPROCESS Process);

typedef PPEB32(NTAPI * PsGetProcessWow64Process)(PEPROCESS Process);

//////////////////////////////////////////////////
//				   Variables					//
//////////////////////////////////////////////////

/**
 * @brief Address of ZwQueryInformationProcess
 * 
 */
ZwQueryInformationProcess g_ZwQueryInformationProcess;

/**
 * @brief Address of PsGetProcessPeb
 * 
 */
PsGetProcessPeb g_PsGetProcessPeb;

/**
 * @brief Address of PsGetProcessWow64Process
 * 
 */
PsGetProcessWow64Process g_PsGetProcessWow64Process;

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

BOOLEAN
UserAccessAllocateAndGetImagePathFromProcessId(HANDLE          ProcessId,
                                               PUNICODE_STRING ProcessImageName,
                                               UINT32          SizeOfImageNameToBeAllocated);
BOOLEAN
UserAccessGetLoadedModules(PUSERMODE_LOADED_MODULE_DETAILS ProcessLoadedModuleRequest, UINT32 BufferSize);

BOOLEAN
UserAccessIsWow64Process(HANDLE ProcessId, PBOOLEAN Is32Bit);

BOOLEAN
UserAccessCheckForLoadedModuleDetails();
