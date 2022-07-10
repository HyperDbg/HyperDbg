/**
 * @file PseudoRegisters.c
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Script engine pseudo-registers implementations
 * @details
 * @version 0.2
 * @date 2022-06-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// *** Pseudo-registers ***
//

/**
 * @brief Implementation of $tid pseudo-register
 *
 * @return UINT64
 */
UINT64
ScriptEnginePseudoRegGetTid()
{
#ifdef SCRIPT_ENGINE_USER_MODE
    return GetCurrentThreadId();
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    return PsGetCurrentThreadId();
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

/**
 * @brief Implementation of $core pseudo-register
 *
 * @return UINT64
 */
UINT64
ScriptEnginePseudoRegGetCore()
{
#ifdef SCRIPT_ENGINE_USER_MODE
    return GetCurrentProcessorNumber();
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    return KeGetCurrentProcessorNumber();
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

/**
 * @brief Implementation of $pid pseudo-register
 *
 * @return UINT64
 */
UINT64
ScriptEnginePseudoRegGetPid()
{
#ifdef SCRIPT_ENGINE_USER_MODE
    return GetCurrentProcessId();
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    return PsGetCurrentProcessId();
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

/**
 * @brief Implementation of $pname pseudo-register
 *
 * @return CHAR*
 */
CHAR *
ScriptEnginePseudoRegGetPname()
{
#ifdef SCRIPT_ENGINE_USER_MODE

    HANDLE Handle = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE,
        GetCurrentProcessId() /* Current process */
    );

    if (Handle)
    {
        CHAR CurrentModulePath[MAX_PATH] = {0};
        if (GetModuleFileNameEx(Handle, 0, CurrentModulePath, MAX_PATH))
        {
            //
            // At this point, buffer contains the full path to the executable
            //
            CloseHandle(Handle);
            return PathFindFileNameA(CurrentModulePath);
        }
        else
        {
            //
            // error might be shown by GetLastError()
            //
            CloseHandle(Handle);
            return NULL;
        }
    }

    //
    // unable to get handle
    //
    return NULL;

#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    return GetProcessNameFromEprocess(PsGetCurrentProcess());
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

/**
 * @brief Implementation of $proc pseudo-register
 *
 * @return UINT64
 */
UINT64
ScriptEnginePseudoRegGetProc()
{
#ifdef SCRIPT_ENGINE_USER_MODE
    return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    return PsGetCurrentProcess();
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

/**
 * @brief Implementation of $thread pseudo-register
 *
 * @return UINT64
 */
UINT64
ScriptEnginePseudoRegGetThread()
{
#ifdef SCRIPT_ENGINE_USER_MODE
    return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    return PsGetCurrentThread();
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

/**
 * @brief Implementation of $peb pseudo-register
 *
 * @return UINT64
 */
UINT64
ScriptEnginePseudoRegGetPeb()
{
#ifdef SCRIPT_ENGINE_USER_MODE
    //
    // Hand-rolled structs ( may cause conflict depending on your dev env )
    //
    struct PROCESS_BASIC_INFORMATION
    {
        PVOID     Reserved1;
        PVOID     PebBaseAddress;
        PVOID     Reserved2[2];
        ULONG_PTR UniqueProcessId;
        PVOID     Reserved3;
    };

    struct PEB_LDR_DATA
    {
        BYTE       Reserved1[8];
        PVOID      Reserved2[3];
        LIST_ENTRY InMemoryOrderModuleList;
    };

    struct PEB
    {
        BYTE                  Reserved1[2];
        BYTE                  BeingDebugged;
        BYTE                  Reserved2[1];
        PVOID                 Reserved3[2];
        struct PEB_LDR_DATA * Ldr;
        PVOID                 ProcessParameters; /* PRTL_USER_PROCESS_PARAMETERS */
        BYTE                  Reserved4[104];
        PVOID                 Reserved5[52];
        PVOID                 PostProcessInitRoutine; /* PPS_POST_PROCESS_INIT_ROUTINE */
        BYTE                  Reserved6[128];
        PVOID                 Reserved7[1];
        ULONG                 SessionId;
    };

    struct UNICODE_STRING
    {
        USHORT Length;
        USHORT MaximumLength;
        PWSTR  Buffer;
    };

    struct LDR_MODULE
    {
        LIST_ENTRY            InLoadOrderModuleList;
        LIST_ENTRY            InMemoryOrderModuleList;
        LIST_ENTRY            InInitializationOrderModuleList;
        PVOID                 BaseAddress;
        PVOID                 EntryPoint;
        ULONG                 SizeOfImage;
        struct UNICODE_STRING FullDllName;
        struct UNICODE_STRING BaseDllName;
        ULONG                 Flags;
        SHORT                 LoadCount;
        SHORT                 TlsIndex;
        LIST_ENTRY            HashTableEntry;
        ULONG                 TimeDateStamp;
    };

    enum PROCESSINFOCLASS
    {
        ProcessBasicInformation = 0,
        ProcessDebugPort        = 7,
        ProcessWow64Information = 26,
        ProcessImageFileName    = 27
    };

    LPCWSTR NTDLL_NAME       = L"ntdll.dll";
    LPCSTR  NTQUERYINFO_NAME = "NtQueryInformationProcess";

    HMODULE  NtdllMod;
    HANDLE   ThisProcess;
    NTSTATUS NtCallRet;
    ULONG    BytesReturned;

    //
    // function pointer to house result from GetProcAddress
    //
    NTSTATUS(WINAPI * QueryInfoProcPtr)
    (HANDLE, enum PROCESSINFOCLASS, PVOID, ULONG, PULONG);

    struct PROCESS_BASIC_INFORMATION BasicInfo;
    struct PEB *                     PebPtr;
    struct LDR_MODULE *              modPtr;

    /* retrieve pseudo-handle */
    ThisProcess = GetCurrentProcess();

    //
    // get address to already loaded module
    //
    NtdllMod = LoadLibraryW(NTDLL_NAME);

    //
    // get pointer to query function
    //
    QueryInfoProcPtr =
        (NTSTATUS(WINAPI *)(HANDLE, enum PROCESSINFOCLASS, PVOID, ULONG, PULONG))GetProcAddress(NtdllMod, NTQUERYINFO_NAME);

    //
    // call function on self; introspect
    //
    NtCallRet = QueryInfoProcPtr(ThisProcess, ProcessBasicInformation, &BasicInfo, sizeof(BasicInfo), &BytesReturned);

    //
    // get peb ptr and decode some if its fields
    //
    PebPtr = (struct PEB *)BasicInfo.PebBaseAddress;

    return (UINT64)PebPtr;

#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    //
    // PEB doesn't make sense in kernel-mode
    //
    return NULL;
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

/**
 * @brief Implementation of $teb pseudo-register
 *
 * @return UINT64
 */
UINT64
ScriptEnginePseudoRegGetTeb()
{
#ifdef SCRIPT_ENGINE_USER_MODE
    return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    return PsGetCurrentThreadTeb();
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

/**
 * @brief Implementation of $ip pseudo-register
 *
 * @return UINT64
 */
UINT64
ScriptEnginePseudoRegGetIp()
{
#ifdef SCRIPT_ENGINE_USER_MODE
    //
    // $ip doesn't have meaning in user-moderds
    //
    return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    return ScriptEngineWrapperGetInstructionPointer();
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

/**
 * @brief Implementation of $buffer pseudo-register
 *
 * @param CorrespondingAction
 * @return UINT64
 */
UINT64
ScriptEnginePseudoRegGetBuffer(UINT64 * CorrespondingAction)
{
#ifdef SCRIPT_ENGINE_USER_MODE
    //
    // $buffer doesn't mean anything in user-mode
    //
    return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    return ScriptEngineWrapperGetAddressOfReservedBuffer(CorrespondingAction);
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

/**
 * @brief Implementation of $event_tag pseudo-register
 * @param ActionBuffer
 *
 * @return UINT64
 */
UINT64
ScriptEnginePseudoRegGetEventTag(PACTION_BUFFER ActionBuffer)
{
#ifdef SCRIPT_ENGINE_USER_MODE
    return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    return ActionBuffer->Tag;
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

/**
 * @brief Implementation of $event_id pseudo-register
 * @param ActionBuffer
 *
 * @return UINT64
 */
UINT64
ScriptEnginePseudoRegGetEventId(PACTION_BUFFER ActionBuffer)
{
#ifdef SCRIPT_ENGINE_USER_MODE
    return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    return (ActionBuffer->Tag - DebuggerEventTagStartSeed);
#endif // SCRIPT_ENGINE_KERNEL_MODE
}
