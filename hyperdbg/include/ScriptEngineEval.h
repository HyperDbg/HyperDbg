/**
 * @file ScriptEngineEval.h
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @author Alee Amini (alee@hyperdbg.org)
 * @brief Shared Headers for Script engine
 * @details
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//
// Basic data types is used for communication in script engine
//
#include "SDK/Headers/Constants.h"
#include "SDK/Headers/BasicTypes.h"

//
// Global Variables
//

/**
 * @brief global variable to save the result of script-engine statement
 * tests
 *
 */
UINT64 g_CurrentExprEvalResult;

/**
 * @brief global variable to detect if there was an error in the result
 *  of script-engine statement tests
 *
 */
BOOLEAN g_CurrentExprEvalResultHasError;

//
// Wrapper headers
//

#ifdef SCRIPT_ENGINE_KERNEL_MODE

UINT64
ScriptEngineWrapperGetInstructionPointer();

UINT64
ScriptEngineWrapperGetAddressOfReservedBuffer(PDEBUGGER_EVENT_ACTION Action);

UINT32
VmxrootCompatibleStrlen(const CHAR * S);

UINT32
VmxrootCompatibleWcslen(const wchar_t * S);

BOOLEAN
MemoryMapperReadMemorySafeOnTargetProcess(UINT64 VaAddressToRead,
                                          PVOID  BufferToSaveMemory,
                                          SIZE_T SizeToRead);

//
// Both user-mode and kernel-mode should implement this function
//
BOOLEAN
CheckMemoryAccessSafety(UINT64 TargetAddress, UINT32 Size);

#endif // SCRIPT_ENGINE_KERNEL_MODE

#ifdef SCRIPT_ENGINE_USER_MODE

BOOLEAN
CheckMemoryAccessSafety(UINT64 TargetAddress, UINT32 Size);

#endif // SCRIPT_ENGINE_USER_MODE

//
// Definition
//
UINT64
GetValue(PGUEST_REGS                    GuestRegs,
         PACTION_BUFFER                 ActionBuffer,
         SCRIPT_ENGINE_VARIABLES_LIST * VariablesList,
         PSYMBOL                        Symbol,
         BOOLEAN                        ReturnReference);
//
// *** Pseudo registers ***
//
// $tid
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

// $core
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

// $pid
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

// $pname
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

// $proc
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

// $thread
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

// $peb
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

// $teb
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

// $ip
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

// $buffer
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

//
// Check whether the address is valid or
//
BOOLEAN
ScriptEngineCheckAddressValidity(UINT64 Address, UINT32 Length)
{
#ifdef SCRIPT_ENGINE_USER_MODE

    if (CheckMemoryAccessSafety(Address, Length))
    {
        return TRUE;
    }

#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    if (CheckMemoryAccessSafety(Address, Length))
    {
        return TRUE;
    }

#endif // SCRIPT_ENGINE_KERNEL_MODE

    return FALSE;
}

//
// Convert virtual address to physical address
//
UINT64
ScriptEngineFunctionVirtualToPhysical(UINT64 Address)
{
#ifdef SCRIPT_ENGINE_USER_MODE

    //
    // There is no conversion in user-mode
    //
    return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    return VirtualAddressToPhysicalAddressOnTargetProcess(Address);

#endif // SCRIPT_ENGINE_KERNEL_MODE
}

//
// Convert physical address to virtual address
//
UINT64
ScriptEngineFunctionPhysicalToVirtual(UINT64 Address)
{
#ifdef SCRIPT_ENGINE_USER_MODE

    //
    // There is no conversion in user-mode
    //
    return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    return PhysicalAddressToVirtualAddressOnTargetProcess(Address);

#endif // SCRIPT_ENGINE_KERNEL_MODE
}

//
// *** Keywords ***
//

// poi
UINT64
ScriptEngineKeywordPoi(PUINT64 Address, BOOL * HasError)
{
    UINT64 Result = NULL;

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    if (!CheckMemoryAccessSafety(Address, sizeof(UINT64)))
    {
        *HasError = TRUE;

        return NULL;
    }

#endif // SCRIPT_ENGINE_KERNEL_MODE

#ifdef SCRIPT_ENGINE_USER_MODE
    Result = *Address;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    MemoryMapperReadMemorySafeOnTargetProcess(Address, &Result, sizeof(UINT64));
#endif // SCRIPT_ENGINE_KERNEL_MODE

    return Result;
}

// hi
WORD
ScriptEngineKeywordHi(PUINT64 Address, BOOL * HasError)
{
    QWORD Result = NULL;

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    if (!CheckMemoryAccessSafety(Address, sizeof(UINT64)))
    {
        *HasError = TRUE;

        return NULL;
    }

#endif // SCRIPT_ENGINE_KERNEL_MODE

#ifdef SCRIPT_ENGINE_USER_MODE
    Result = *Address;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    MemoryMapperReadMemorySafeOnTargetProcess(Address, &Result, sizeof(UINT64));
#endif // SCRIPT_ENGINE_KERNEL_MODE

    return HIWORD(Result);
}

// low
WORD
ScriptEngineKeywordLow(PUINT64 Address, BOOL * HasError)
{
    QWORD Result = NULL;

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    if (!CheckMemoryAccessSafety(Address, sizeof(UINT64)))
    {
        *HasError = TRUE;

        return NULL;
    }

#endif // SCRIPT_ENGINE_KERNEL_MODE

#ifdef SCRIPT_ENGINE_USER_MODE
    Result = *Address;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    MemoryMapperReadMemorySafeOnTargetProcess(Address, &Result, sizeof(UINT64));
#endif // SCRIPT_ENGINE_KERNEL_MODE

    return LOWORD(Result);
}

// db
BYTE
ScriptEngineKeywordDb(PUINT64 Address, BOOL * HasError)
{
    BYTE Result = NULL;

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    if (!CheckMemoryAccessSafety(Address, sizeof(BYTE)))
    {
        *HasError = TRUE;

        return NULL;
    }

#endif // SCRIPT_ENGINE_KERNEL_MODE

#ifdef SCRIPT_ENGINE_USER_MODE
    Result = *Address;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    MemoryMapperReadMemorySafeOnTargetProcess(Address, &Result, sizeof(BYTE));
#endif // SCRIPT_ENGINE_KERNEL_MODE

    return Result;
}

// dd
WORD
ScriptEngineKeywordDd(PUINT64 Address, BOOL * HasError)
{
    WORD Result = NULL;

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    if (!CheckMemoryAccessSafety(Address, sizeof(WORD)))
    {
        *HasError = TRUE;

        return NULL;
    }

#endif // SCRIPT_ENGINE_KERNEL_MODE

#ifdef SCRIPT_ENGINE_USER_MODE
    Result = *Address;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    MemoryMapperReadMemorySafeOnTargetProcess(Address, &Result, sizeof(WORD));
#endif // SCRIPT_ENGINE_KERNEL_MODE

    return Result;
}

// dw
DWORD
ScriptEngineKeywordDw(PUINT64 Address, BOOL * HasError)
{
    DWORD Result = NULL;

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    if (!CheckMemoryAccessSafety(Address, sizeof(DWORD)))
    {
        *HasError = TRUE;

        return NULL;
    }

#endif // SCRIPT_ENGINE_KERNEL_MODE

#ifdef SCRIPT_ENGINE_USER_MODE
    Result = *Address;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    MemoryMapperReadMemorySafeOnTargetProcess(Address, &Result, sizeof(DWORD));
#endif // SCRIPT_ENGINE_KERNEL_MODE

    return Result;
}

// dq
QWORD
ScriptEngineKeywordDq(PUINT64 Address, BOOL * HasError)
{
    QWORD Result = NULL;

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    if (!CheckMemoryAccessSafety(Address, sizeof(DWORD)))
    {
        *HasError = TRUE;

        return NULL;
    }

#endif // SCRIPT_ENGINE_KERNEL_MODE

#ifdef SCRIPT_ENGINE_USER_MODE
    Result = *Address;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    MemoryMapperReadMemorySafeOnTargetProcess(Address, &Result, sizeof(QWORD));
#endif // SCRIPT_ENGINE_KERNEL_MODE

    return Result;
}

//
// *** Functions ***
//

// eq
BOOLEAN
ScriptEngineFunctionEq(UINT64 Address, QWORD Value, BOOL * HasError)
{
#ifdef SCRIPT_ENGINE_KERNEL_MODE

    if (!CheckMemoryAccessSafety(Address, sizeof(QWORD)))
    {
        //
        // Instead of indicating an error, just return false
        // to assign it as a return result to a variable
        //
        // *HasError = TRUE;

        return FALSE;
    }

#endif // SCRIPT_ENGINE_KERNEL_MODE

#ifdef SCRIPT_ENGINE_USER_MODE
    *(UINT64 *)Address = Value;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    MemoryMapperWriteMemorySafeOnTargetProcess(Address, &Value, sizeof(QWORD));
#endif // SCRIPT_ENGINE_KERNEL_MODE

    return TRUE;
}

// ed
BOOLEAN
ScriptEngineFunctionEd(UINT64 Address, DWORD Value, BOOL * HasError)
{
#ifdef SCRIPT_ENGINE_KERNEL_MODE

    if (!CheckMemoryAccessSafety(Address, sizeof(DWORD)))
    {
        //
        // Instead of indicating an error, just return false
        // to assign it as a return result to a variable
        //
        // *HasError = TRUE;

        return FALSE;
    }

#endif // SCRIPT_ENGINE_KERNEL_MODE

#ifdef SCRIPT_ENGINE_USER_MODE
    *(DWORD *)Address = Value;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    MemoryMapperWriteMemorySafeOnTargetProcess(Address, &Value, sizeof(DWORD));
#endif // SCRIPT_ENGINE_KERNEL_MODE

    return TRUE;
}

// eb
BOOLEAN
ScriptEngineFunctionEb(UINT64 Address, BYTE Value, BOOL * HasError)
{
#ifdef SCRIPT_ENGINE_KERNEL_MODE

    if (!CheckMemoryAccessSafety(Address, sizeof(BYTE)))
    {
        //
        // Instead of indicating an error, just return false
        // to assign it as a return result to a variable
        //
        // *HasError = TRUE;

        return FALSE;
    }

#endif // SCRIPT_ENGINE_KERNEL_MODE

#ifdef SCRIPT_ENGINE_USER_MODE
    *(BYTE *)Address = Value;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    MemoryMapperWriteMemorySafeOnTargetProcess(Address, &Value, sizeof(BYTE));
#endif // SCRIPT_ENGINE_KERNEL_MODE

    return TRUE;
}

// print
VOID
ScriptEngineFunctionPrint(UINT64 Tag, BOOLEAN ImmediateMessagePassing, UINT64 Value)
{
#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("%llx\n", Value);
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    //
    // Prepare a buffer to bypass allocating a huge stack space for logging
    //
    char   TempBuffer[20] = {0};
    UINT32 TempBufferLen  = sprintf(TempBuffer, "%llx", Value);

    LogSimpleWithTag(Tag, ImmediateMessagePassing, TempBuffer, TempBufferLen + 1);

#endif // SCRIPT_ENGINE_KERNEL_MODE
}

VOID
ScriptEngineFunctionTestStatement(UINT64 Tag, BOOLEAN ImmediateMessagePassing, UINT64 Value)
{
    g_CurrentExprEvalResult         = Value;
    g_CurrentExprEvalResultHasError = FALSE;
}

//
// Spinlock functions
//

// spinlock_lock
VOID
ScriptEngineFunctionSpinlockLock(volatile LONG * Lock, BOOL * HasError)
{
#ifdef SCRIPT_ENGINE_USER_MODE

    SpinlockLock(Lock);

#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    if (!CheckMemoryAccessSafety(Lock, sizeof(LONG)))
    {
        *HasError = TRUE;
        return;
    }

    SpinlockLock(Lock);

#endif // SCRIPT_ENGINE_KERNEL_MODE
}

// spinlock_unlock
VOID
ScriptEngineFunctionSpinlockUnlock(volatile LONG * Lock, BOOL * HasError)
{
#ifdef SCRIPT_ENGINE_USER_MODE

    SpinlockUnlock(Lock);

#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    if (!CheckMemoryAccessSafety(Lock, sizeof(LONG)))
    {
        *HasError = TRUE;
        return;
    }

    SpinlockUnlock(Lock);

#endif // SCRIPT_ENGINE_KERNEL_MODE
}

// spinlock_lock_custom_wait
VOID
ScriptEngineFunctionSpinlockLockCustomWait(volatile long * Lock, unsigned MaxWait, BOOL * HasError)
{
#ifdef SCRIPT_ENGINE_USER_MODE

    SpinlockLockWithCustomWait(Lock, MaxWait);

#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    if (!CheckMemoryAccessSafety(Lock, sizeof(LONG)))
    {
        *HasError = TRUE;
        return;
    }

    SpinlockLockWithCustomWait(Lock, MaxWait);

#endif // SCRIPT_ENGINE_KERNEL_MODE
}

//
// String length functions
//

// strlen
UINT64
ScriptEngineFunctionStrlen(const char * Address)
{
    UINT64 Result = 0;
#ifdef SCRIPT_ENGINE_USER_MODE
    Result = strlen(Address);
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    Result = VmxrootCompatibleStrlen(Address);
#endif // SCRIPT_ENGINE_KERNEL_MODE

    return Result;
}

// wcslen
UINT64
ScriptEngineFunctionWcslen(const wchar_t * Address)
{
    UINT64 Result = 0;

#ifdef SCRIPT_ENGINE_USER_MODE
    Result = wcslen(Address);
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    Result = VmxrootCompatibleWcslen(Address);
#endif // SCRIPT_ENGINE_KERNEL_MODE

    return Result;
}

//
// Interlocked atomic functions
//

// interlocked_exchange
long long
ScriptEngineFunctionInterlockedExchange(long long volatile * Target,
                                        long long            Value,
                                        BOOL *               HasError)
{
    long long Result = 0;

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    if (!CheckMemoryAccessSafety(Target, sizeof(long long)))
    {
        *HasError = TRUE;
        return NULL;
    }

#endif // SCRIPT_ENGINE_KERNEL_MODE

    Result = InterlockedExchange64(Target, Value);

    return Result;
}

// interlocked_exchange_add
long long
ScriptEngineFunctionInterlockedExchangeAdd(long long volatile * Addend,
                                           long long            Value,
                                           BOOL *               HasError)
{
    long long Result = 0;

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    if (!CheckMemoryAccessSafety(Addend, sizeof(long long)))
    {
        *HasError = TRUE;
        return NULL;
    }

#endif // SCRIPT_ENGINE_KERNEL_MODE

    Result = InterlockedExchangeAdd64(Addend, Value);

    return Result;
}

// interlocked_exchange_increment
long long
ScriptEngineFunctionInterlockedIncrement(long long volatile * Addend,
                                         BOOL *               HasError)
{
    long long Result = 0;

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    if (!CheckMemoryAccessSafety(Addend, sizeof(long long)))
    {
        *HasError = TRUE;
        return NULL;
    }

#endif // SCRIPT_ENGINE_KERNEL_MODE

    Result = InterlockedIncrement64(Addend);

    return Result;
}

// interlocked_exchange_decrement
long long
ScriptEngineFunctionInterlockedDecrement(long long volatile * Addend,
                                         BOOL *               HasError)
{
    long long Result = 0;

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    if (!CheckMemoryAccessSafety(Addend, sizeof(long long)))
    {
        *HasError = TRUE;
        return NULL;
    }

#endif // SCRIPT_ENGINE_KERNEL_MODE

    Result = InterlockedDecrement64(Addend);

    return Result;
}

// interlocked_compare_exchange
long long
ScriptEngineFunctionInterlockedCompareExchange(
    long long volatile * Destination,
    long long            ExChange,
    long long            Comperand,
    BOOL *               HasError)
{
    long long Result = 0;

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    if (!CheckMemoryAccessSafety(Destination, sizeof(long long)))
    {
        *HasError = TRUE;
        return NULL;
    }

#endif // SCRIPT_ENGINE_KERNEL_MODE

    Result = InterlockedCompareExchange64(Destination, ExChange, Comperand);

    return Result;
}

// enable_event
VOID
ScriptEngineFunctionEnableEvent(UINT64  Tag,
                                BOOLEAN ImmediateMessagePassing,
                                UINT64  Value)
{
#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("err, enabling events is not possible in user-mode\n");
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    if (!DebuggerEnableEvent(Value + DebuggerEventTagStartSeed))
    {
        LogInfo("Invalid tag id (%x)", Value);
    }
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

// disable_event
VOID
ScriptEngineFunctionDisableEvent(UINT64  Tag,
                                 BOOLEAN ImmediateMessagePassing,
                                 UINT64  Value)
{
#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("err, disabling events is not possible in user-mode\n");
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    if (!DebuggerDisableEvent(Value + DebuggerEventTagStartSeed))
    {
        LogInfo("Invalid tag id (%x)", Value);
    }
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

// pause
VOID
ScriptEngineFunctionPause(UINT64 Tag, BOOLEAN ImmediateMessagePassing, PGUEST_REGS GuestRegs, UINT64 Context)
{
#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("err, breaking is not possible in user-mode\n");
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    //
    // pause(); function is only working when kernel debugger is working
    // it's not designed to work on vmi-mode (local debugging)
    //
    if (g_KernelDebuggerState && g_DebuggeeHaltReason == DEBUGGEE_PAUSING_REASON_NOT_PAUSED)
    {
        DEBUGGER_TRIGGERED_EVENT_DETAILS ContextAndTag         = {0};
        UINT32                           CurrentProcessorIndex = KeGetCurrentProcessorNumber();

        if (g_GuestState[CurrentProcessorIndex].IsOnVmxRootMode)
        {
            //
            // The guest is already in vmx-root mode
            // Halt other cores
            //
            ContextAndTag.Tag     = Tag;
            ContextAndTag.Context = Context;

            KdHandleBreakpointAndDebugBreakpoints(
                CurrentProcessorIndex,
                GuestRegs,
                DEBUGGEE_PAUSING_REASON_DEBUGGEE_EVENT_TRIGGERED,
                &ContextAndTag);
        }
        else
        {
            //
            // The guest is on vmx non-root mode, the first parameter
            // is context and the second parameter is tag
            //
            AsmVmxVmcall(VMCALL_VM_EXIT_HALT_SYSTEM_AS_A_RESULT_OF_TRIGGERING_EVENT, Context, Tag, GuestRegs);
        }
    }
    else
    {
        LogInfo("The 'pause();' function is either called from the vmi-mode or is "
                "evaluated by the '?' command. It's not allowed to use it on vmi-mode "
                "(local debugging) or by the '?' command");
    }

#endif // SCRIPT_ENGINE_KERNEL_MODE
}

// flush
VOID
ScriptEngineFunctionFlush()
{
#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("err, it's not possible to flush buffers in user-mode\n");
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    //
    // Mark all buffers as read
    //
    LogMarkAllAsRead(TRUE);
    LogMarkAllAsRead(FALSE);

#endif // SCRIPT_ENGINE_KERNEL_MODE
}

// event_ignore
VOID
ScriptEngineFunctionEventIgnore()
{
#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("err, it's not possible to ignore events in user-mode\n");
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    UINT32 CurrentProcessorIndex                                   = KeGetCurrentProcessorNumber();
    g_GuestState[CurrentProcessorIndex].DebuggingState.IgnoreEvent = TRUE;

#endif // SCRIPT_ENGINE_KERNEL_MODE
}

VOID
ScriptEngineFunctionFormats(UINT64 Tag, BOOLEAN ImmediateMessagePassing, UINT64 Value)
{
#ifdef SCRIPT_ENGINE_USER_MODE

    g_CurrentExprEvalResult         = Value;
    g_CurrentExprEvalResultHasError = FALSE;

#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    if (g_KernelDebuggerState)
    {
        KdSendFormatsFunctionResult(Value);
    }
    else
    {
        //
        // Prepare a buffer to bypass allocating a huge stack space for logging
        //
        char   TempBuffer[20] = {0};
        UINT32 TempBufferLen  = sprintf(TempBuffer, "%llx\n", Value);

        LogSimpleWithTag(Tag, ImmediateMessagePassing, TempBuffer, TempBufferLen + 1);
    }
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

UINT32
CustomStrlen(UINT64 StrAddr, BOOLEAN IsWstring)
{
#ifdef SCRIPT_ENGINE_USER_MODE

    if (IsWstring)
    {
        return wcslen((const wchar_t *)StrAddr);
    }
    else
    {
        return strlen((const char *)StrAddr);
    }
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
    if (IsWstring)
    {
        return VmxrootCompatibleWcslen((const wchar_t *)StrAddr);
    }
    else
    {
        return VmxrootCompatibleStrlen((const CHAR *)StrAddr);
    }
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

BOOLEAN
CheckIfStringIsSafe(UINT64 StrAddr, BOOLEAN IsWstring)
{
#ifdef SCRIPT_ENGINE_USER_MODE
    return TRUE;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    //
    // At least two chars (wchar_t is 4 byte)
    //
    if (CheckMemoryAccessSafety(StrAddr, IsWstring ? 4 : 2))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

VOID
ApplyFormatSpecifier(const CHAR * CurrentSpecifier, CHAR * FinalBuffer, PUINT32 CurrentProcessedPositionFromStartOfFormat, PUINT32 CurrentPositionInFinalBuffer, UINT64 Val, UINT32 SizeOfFinalBuffer)
{
    UINT32 TempBufferLen      = 0;
    CHAR   TempBuffer[50 + 1] = {
          0}; // Maximum uint64_t is 18446744073709551615 + 1 thus its 20 character
              // for maximum buffer + 1 end char null but we alloc 50 to be sure

    *CurrentProcessedPositionFromStartOfFormat =
        *CurrentProcessedPositionFromStartOfFormat + strlen(CurrentSpecifier);
    sprintf(TempBuffer, CurrentSpecifier, Val);
    TempBufferLen = strlen(TempBuffer);

    //
    // Check final buffer capacity
    //
    if (*CurrentPositionInFinalBuffer + TempBufferLen > SizeOfFinalBuffer)
    {
        //
        // Over passed buffer
        //
        return;
    }

    memcpy(&FinalBuffer[*CurrentPositionInFinalBuffer], TempBuffer, TempBufferLen);

    *CurrentPositionInFinalBuffer = *CurrentPositionInFinalBuffer + TempBufferLen;
}

size_t
WcharToChar(const wchar_t * src, char * dest, size_t dest_len)
{
    wchar_t Code;
    size_t  i;

    i = 0;

    while ((src[i] != '\0') && i < (dest_len - 1))
    {
        Code = src[i];
        if (Code < 128)
            dest[i] = (char)Code;
        else
        {
            dest[i] = '?';
            if (Code >= 0xD800 && Code <= 0xD8FF)
            {
                //
                // Lead surrogate, skip the next code unit, which is the trail
                //
                i++;
            }
        }
        i++;
    }

    return i - 1;
}

BOOLEAN
ApplyStringFormatSpecifier(const CHAR * CurrentSpecifier, CHAR * FinalBuffer, PUINT32 CurrentProcessedPositionFromStartOfFormat, PUINT32 CurrentPositionInFinalBuffer, UINT64 Val, BOOLEAN IsWstring, UINT32 SizeOfFinalBuffer)
{
    UINT32  StringSize;
    wchar_t WstrBuffer[50];
    CHAR    AsciiBuffer[sizeof(WstrBuffer) / 2];
    UINT32  StringSizeInByte; /* because of wide-char */
    UINT32  CountOfBlocks;
    UINT32  CountOfBytesToRead;
    UINT32  CopiedBlockLen;

    //
    // First we have to check if string is valid or not
    //
    if (!CheckIfStringIsSafe(Val, IsWstring))
    {
        return FALSE;
    }

    //
    // get the length of the string (format) identifier
    //
    *CurrentProcessedPositionFromStartOfFormat += strlen(CurrentSpecifier);

    //
    // Get string len
    //
    StringSize = CustomStrlen(Val, IsWstring);

    //
    // Check final buffer capacity
    //
    if (*CurrentPositionInFinalBuffer + StringSize > SizeOfFinalBuffer)
    {
        //
        // Over passed buffer
        //
        return TRUE;
    }

    //
    // Move the buffer string into the target buffer
    //
    if (IsWstring)
    {
        //
        // Parse wstring
        //
        StringSizeInByte = StringSize * 2; /* because of wide-char */

        //
        // compute the ceiling
        //
        if (StringSizeInByte % sizeof(WstrBuffer) == 0)
        {
            CountOfBlocks = StringSizeInByte / sizeof(WstrBuffer);
        }
        else
        {
            CountOfBlocks = (StringSizeInByte / sizeof(WstrBuffer)) + 1;
        }

        for (size_t i = 0; i < CountOfBlocks; i++)
        {
            //
            // Zero the buffers
            //
            RtlZeroMemory(WstrBuffer, sizeof(WstrBuffer));
            RtlZeroMemory(AsciiBuffer, sizeof(AsciiBuffer));

            //
            // Check for the last block
            //
            if (i == CountOfBlocks - 1)
            {
                //
                // A portion of block
                //

#ifdef SCRIPT_ENGINE_USER_MODE
                memcpy(WstrBuffer, (void *)(Val + (i * sizeof(WstrBuffer))), StringSizeInByte % sizeof(WstrBuffer));
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
                MemoryMapperReadMemorySafeOnTargetProcess(
                    (void *)(Val + (i * sizeof(WstrBuffer))),
                    WstrBuffer,
                    StringSizeInByte % sizeof(WstrBuffer));
#endif // SCRIPT_ENGINE_KERNEL_MODE
            }
            else
            {
                //
                // A complete block
                //

#ifdef SCRIPT_ENGINE_USER_MODE
                memcpy(WstrBuffer, (void *)(Val + (i * sizeof(WstrBuffer))), sizeof(WstrBuffer));
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
                MemoryMapperReadMemorySafeOnTargetProcess(
                    (void *)(Val + (i * sizeof(WstrBuffer))),
                    WstrBuffer,
                    sizeof(WstrBuffer));
#endif // SCRIPT_ENGINE_KERNEL_MODE
            }

            //
            // Here we have the filled WstrBuffer
            // We should convert WstrBuffer to AsciiBuffer
            //
            CopiedBlockLen =
                WcharToChar(WstrBuffer, AsciiBuffer, sizeof(AsciiBuffer) + 1);

            //
            // Now we should move the AsciiBuffer to the target buffer
            // (when we filled AsciiBuffer the memory here is safe so we
            // can use memcpy in both user-mode and vmx-root mode)
            //
            memcpy(&FinalBuffer[*CurrentPositionInFinalBuffer], (void *)AsciiBuffer, CopiedBlockLen + 1);

            *CurrentPositionInFinalBuffer += CopiedBlockLen + 1;
        }
    }
    else
    {
        //
        // Parse string
        //
#ifdef SCRIPT_ENGINE_USER_MODE
        memcpy(&FinalBuffer[*CurrentPositionInFinalBuffer], (void *)Val, StringSize);
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        MemoryMapperReadMemorySafeOnTargetProcess(
            Val,
            &FinalBuffer[*CurrentPositionInFinalBuffer],
            StringSize);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        *CurrentPositionInFinalBuffer += StringSize;
    }

    return TRUE;
}

VOID
ScriptEngineFunctionPrintf(PGUEST_REGS                    GuestRegs,
                           ACTION_BUFFER *                ActionDetail,
                           SCRIPT_ENGINE_VARIABLES_LIST * VariablesList,
                           UINT64                         Tag,
                           BOOLEAN                        ImmediateMessagePassing,
                           char *                         Format,
                           UINT64                         ArgCount,
                           PSYMBOL                        FirstArg,
                           BOOLEAN *                      HasError)
{
    //
    // The printf function
    //

    char    FinalBuffer[PacketChunkSize]              = {0};
    UINT32  CurrentPositionInFinalBuffer              = 0;
    UINT32  CurrentProcessedPositionFromStartOfFormat = 0;
    BOOLEAN WithoutAnyFormatSpecifier                 = TRUE;

    UINT64  Val;
    UINT32  Position;
    UINT32  LenOfFormats = strlen(Format) + 1;
    PSYMBOL Symbol;

    *HasError = FALSE;

    for (int i = 0; i < ArgCount; i++)
    {
        WithoutAnyFormatSpecifier = FALSE;
        Symbol                    = FirstArg + i;

        //
        // Address is either wstring (%ws) or string (%s)
        //

        Position = (Symbol->Type >> 32) + 1;

        SYMBOL TempSymbol = {0};
        memcpy(&TempSymbol, Symbol, sizeof(SYMBOL));
        TempSymbol.Type &= 0x7fffffff;

        Val = GetValue(GuestRegs, ActionDetail, VariablesList, &TempSymbol, FALSE);

        CHAR PercentageChar = Format[Position];

        /*
    printf("position = %d is %c%c \n", Position, PercentageChar,
           IndicatorChar1);
           */

        if (CurrentProcessedPositionFromStartOfFormat != Position)
        {
            //
            // There is some strings before this format specifier
            // we should move it to the buffer
            //
            UINT32 StringLen = Position - CurrentProcessedPositionFromStartOfFormat;

            //
            // Check final buffer capacity
            //
            if (CurrentPositionInFinalBuffer + StringLen < sizeof(FinalBuffer))
            {
                memcpy(&FinalBuffer[CurrentPositionInFinalBuffer],
                       &Format[CurrentProcessedPositionFromStartOfFormat],
                       StringLen);

                CurrentProcessedPositionFromStartOfFormat += StringLen;
                CurrentPositionInFinalBuffer += StringLen;
            }
        }

        //
        // Double check and apply
        //
        if (PercentageChar == '%')
        {
            //
            // Set first character of specifier
            //
            CHAR FormatSpecifier[5] = {0};
            FormatSpecifier[0]      = '%';

            //
            // Read second char
            //
            CHAR IndicatorChar2 = Format[Position + 1];

            //
            // Check if IndicatorChar2 is 2 character long or more
            //
            if (IndicatorChar2 == 'l' || IndicatorChar2 == 'w' ||
                IndicatorChar2 == 'h')
            {
                //
                // Set second char in format specifier
                //
                FormatSpecifier[1] = IndicatorChar2;

                if (IndicatorChar2 == 'l' && Format[Position + 2] == 'l')
                {
                    //
                    // Set third character in format specifier "ll"
                    //
                    FormatSpecifier[2] = 'l';

                    //
                    // Set last character
                    //
                    FormatSpecifier[3] = Format[Position + 3];
                }
                else
                {
                    //
                    // Set last character
                    //
                    FormatSpecifier[2] = Format[Position + 2];
                }
            }
            else
            {
                //
                // It's a one char specifier (Set last character)
                //
                FormatSpecifier[1] = IndicatorChar2;
            }

            //
            // Apply the specifier
            //
            if (!strncmp(FormatSpecifier, "%s", 2))
            {
                //
                // for string
                //
                if (!ApplyStringFormatSpecifier(
                        "%s",
                        FinalBuffer,
                        &CurrentProcessedPositionFromStartOfFormat,
                        &CurrentPositionInFinalBuffer,
                        Val,
                        FALSE,
                        sizeof(FinalBuffer)))
                {
                    *HasError = TRUE;
                    return;
                }
            }
            else if (!strncmp(FormatSpecifier, "%ls", 3) ||
                     !strncmp(FormatSpecifier, "%ws", 3))
            {
                //
                // for wide string (not important if %ls or %ws , only the length is
                // important)
                //
                if (!ApplyStringFormatSpecifier(
                        "%ws",
                        FinalBuffer,
                        &CurrentProcessedPositionFromStartOfFormat,
                        &CurrentPositionInFinalBuffer,
                        Val,
                        TRUE,
                        sizeof(FinalBuffer)))
                {
                    *HasError = TRUE;
                    return;
                }
            }
            else
            {
                ApplyFormatSpecifier(FormatSpecifier, FinalBuffer, &CurrentProcessedPositionFromStartOfFormat, &CurrentPositionInFinalBuffer, Val, sizeof(FinalBuffer));
            }
        }
    }

    if (WithoutAnyFormatSpecifier)
    {
        //
        // Means that it's just a simple print without any format specifier
        //
        if (LenOfFormats < sizeof(FinalBuffer))
        {
            memcpy(FinalBuffer, Format, LenOfFormats);
        }
    }
    else
    {
        //
        // Check if there is anything after the last format specifier
        //
        if (LenOfFormats > CurrentProcessedPositionFromStartOfFormat)
        {
            UINT32 RemainedLen =
                LenOfFormats - CurrentProcessedPositionFromStartOfFormat;

            if (CurrentPositionInFinalBuffer + RemainedLen < sizeof(FinalBuffer))
            {
                memcpy(&FinalBuffer[CurrentPositionInFinalBuffer],
                       &Format[CurrentProcessedPositionFromStartOfFormat],
                       RemainedLen);
            }
        }
    }

//
// Print final result
//
#ifdef SCRIPT_ENGINE_USER_MODE
    printf("%s", FinalBuffer);
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

    //
    // Prepare a buffer to bypass allocating a huge stack space for logging
    //
    LogSimpleWithTag(Tag, ImmediateMessagePassing, FinalBuffer, strlen(FinalBuffer) + 1);

#endif // SCRIPT_ENGINE_KERNEL_MODE
}

UINT64
GetRegValue(PGUEST_REGS GuestRegs, REGS_ENUM RegId)
{
    switch (RegId)
    {
    case REGISTER_RAX:
        return GuestRegs->rax;

        break;

    case REGISTER_EAX:
        return (GuestRegs->rax & LOWER_32_BITS);

        break;

    case REGISTER_AX:
        return (GuestRegs->rax & LOWER_16_BITS);

        break;

    case REGISTER_AH:
        return (GuestRegs->rax & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_AL:
        return (GuestRegs->rax & LOWER_8_BITS);

        break;

    case REGISTER_RCX:
        return GuestRegs->rcx;

        break;

    case REGISTER_ECX:
        return (GuestRegs->rcx & LOWER_32_BITS);

        break;

    case REGISTER_CX:
        return (GuestRegs->rcx & LOWER_16_BITS);

        break;

    case REGISTER_CH:
        return (GuestRegs->rcx & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_CL:
        return (GuestRegs->rcx & LOWER_8_BITS);

        break;

    case REGISTER_RDX:
        return GuestRegs->rdx;

        break;

    case REGISTER_EDX:
        return (GuestRegs->rdx & LOWER_32_BITS);

        break;

    case REGISTER_DX:
        return (GuestRegs->rdx & LOWER_16_BITS);

        break;

    case REGISTER_DH:
        return (GuestRegs->rdx & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_DL:
        return (GuestRegs->rdx & LOWER_8_BITS);

        break;

    case REGISTER_RBX:
        return GuestRegs->rbx;

        break;

    case REGISTER_EBX:
        return (GuestRegs->rbx & LOWER_32_BITS);

        break;

    case REGISTER_BX:
        return (GuestRegs->rbx & LOWER_16_BITS);

        break;

    case REGISTER_BH:
        return (GuestRegs->rbx & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_BL:
        return (GuestRegs->rbx & LOWER_8_BITS);

        break;

    case REGISTER_RSP:
        return GuestRegs->rsp;

        break;

    case REGISTER_ESP:
        return (GuestRegs->rsp & LOWER_32_BITS);

        break;

    case REGISTER_SP:
        return (GuestRegs->rsp & LOWER_16_BITS);

        break;

    case REGISTER_SPL:
        return (GuestRegs->rsp & LOWER_8_BITS);

        break;

    case REGISTER_RBP:
        return GuestRegs->rbp;

        break;

    case REGISTER_EBP:
        return (GuestRegs->rbp & LOWER_32_BITS);

        break;

    case REGISTER_BP:
        return (GuestRegs->rbp & LOWER_16_BITS);

        break;
    case REGISTER_BPL:
        return (GuestRegs->rbp & LOWER_8_BITS);

        break;

    case REGISTER_RSI:
        return GuestRegs->rsi;

        break;

    case REGISTER_ESI:
        return (GuestRegs->rsi & LOWER_32_BITS);

        break;

    case REGISTER_SI:
        return (GuestRegs->rsi & LOWER_16_BITS);

        break;

    case REGISTER_SIL:
        return (GuestRegs->rsi & LOWER_8_BITS);

        break;

    case REGISTER_RDI:
        return GuestRegs->rdi;

        break;

    case REGISTER_EDI:
        return (GuestRegs->rdi & LOWER_32_BITS);

        break;

    case REGISTER_DI:
        return (GuestRegs->rdi & LOWER_16_BITS);

        break;

    case REGISTER_DIL:
        return (GuestRegs->rdi & LOWER_8_BITS);

        break;

    case REGISTER_R8:
        return GuestRegs->r8;

        break;

    case REGISTER_R8D:
        return (GuestRegs->r8 & LOWER_32_BITS);

        break;

    case REGISTER_R8W:
        return (GuestRegs->r8 & LOWER_16_BITS);

        break;

    case REGISTER_R8H:
        return (GuestRegs->r8 & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_R8L:
        return (GuestRegs->r8 & LOWER_8_BITS);

        break;
    case REGISTER_R9:
        return GuestRegs->r9;

        break;

    case REGISTER_R9D:
        return (GuestRegs->r9 & LOWER_32_BITS);

        break;

    case REGISTER_R9W:
        return (GuestRegs->r9 & LOWER_16_BITS);

        break;

    case REGISTER_R9H:
        return (GuestRegs->r9 & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_R9L:
        return (GuestRegs->r9 & LOWER_8_BITS);

        break;

    case REGISTER_R10:
        return GuestRegs->r10;

        break;

    case REGISTER_R10D:
        return (GuestRegs->r10 & LOWER_32_BITS);

        break;

    case REGISTER_R10W:
        return (GuestRegs->r10 & LOWER_16_BITS);

        break;

    case REGISTER_R10H:
        return (GuestRegs->r10 & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_R10L:
        return (GuestRegs->r10 & LOWER_8_BITS);

        break;

    case REGISTER_R11:
        return GuestRegs->r11;

        break;

    case REGISTER_R11D:
        return (GuestRegs->r11 & LOWER_32_BITS);

        break;

    case REGISTER_R11W:
        return (GuestRegs->r11 & LOWER_16_BITS);

        break;

    case REGISTER_R11H:
        return (GuestRegs->r11 & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_R11L:
        return (GuestRegs->r11 & LOWER_8_BITS);

        break;

    case REGISTER_R12:
        return GuestRegs->r12;

        break;

    case REGISTER_R12D:
        return (GuestRegs->r12 & LOWER_32_BITS);

        break;

    case REGISTER_R12W:
        return (GuestRegs->r12 & LOWER_16_BITS);

        break;

    case REGISTER_R12H:
        return (GuestRegs->r12 & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_R12L:
        return (GuestRegs->r12 & LOWER_8_BITS);

        break;

    case REGISTER_R13:
        return GuestRegs->r13;

        break;

    case REGISTER_R13D:
        return (GuestRegs->r13 & LOWER_32_BITS);

        break;

    case REGISTER_R13W:
        return (GuestRegs->r13 & LOWER_16_BITS);

        break;

    case REGISTER_R13H:
        return (GuestRegs->r13 & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_R13L:
        return (GuestRegs->r13 & LOWER_8_BITS);

        break;

    case REGISTER_R14:
        return GuestRegs->r14;

        break;

    case REGISTER_R14D:
        return (GuestRegs->r14 & LOWER_32_BITS);

        break;

    case REGISTER_R14W:
        return (GuestRegs->r14 & LOWER_16_BITS);

        break;

    case REGISTER_R14H:
        return (GuestRegs->r14 & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_R14L:
        return (GuestRegs->r14 & LOWER_8_BITS);

        break;

    case REGISTER_R15:
        return GuestRegs->r15;

        break;

    case REGISTER_R15D:
        return (GuestRegs->r15 & LOWER_32_BITS);

        break;

    case REGISTER_R15W:
        return (GuestRegs->r15 & LOWER_16_BITS);

        break;

    case REGISTER_R15H:
        return (GuestRegs->r15 & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_R15L:
        return (GuestRegs->r15 & LOWER_8_BITS);

        break;

    case REGISTER_DS:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestDs().Selector;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_ES:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestEs().Selector;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_FS:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestFs().Selector;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_GS:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestGs().Selector;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CS:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestCs().Selector;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_SS:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestSs().Selector;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_RFLAGS:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestRFlags();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_EFLAGS:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & LOWER_32_BITS);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_FLAGS:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & LOWER_16_BITS);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & X86_FLAGS_CF) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_PF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_PF)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_AF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_AF)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_ZF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_ZF)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_SF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_SF)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_TF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_TF)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_IF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_IF)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_DF)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_OF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_OF)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_IOPL:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return ((GetGuestRFlags() & (0b11 << X86_FLAGS_IOPL_SHIFT)) >> 12);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_NT:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_NT)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_RF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_RF)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_VM:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_VM)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_AC:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_AC)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_VIF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_VIF)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_VIP:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_VIP)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_ID:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_ID)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_RIP:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestRIP();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_EIP:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRIP() & LOWER_32_BITS);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_IP:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRIP() & LOWER_16_BITS);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_IDTR:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

        return GetGuestIdtr();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_LDTR:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

        return GetGuestLdtr();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_TR:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

        return GetGuestTr();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_GDTR:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestGdtr();

#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CR0:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestCr0();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CR2:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestCr2();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CR3:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestCr3();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CR4:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestCr4();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CR8:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestCr8();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR0:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestDr0();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR1:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestDr1();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR2:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestDr2();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR3:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestDr3();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR6:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestDr6();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR7:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestDr7();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case INVALID:

#ifdef SCRIPT_ENGINE_USER_MODE
        ShowMessages("error in reading register");
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        LogInfo("Error in reading register");
#endif // SCRIPT_ENGINE_KERNEL_MODE

        return INVALID;

        break;
    }
}
UINT64
GetPseudoRegValue(PSYMBOL Symbol, PACTION_BUFFER ActionBuffer)
{
    switch (Symbol->Value)
    {
    case PSEUDO_REGISTER_TID:
        return ScriptEnginePseudoRegGetTid();
    case PSEUDO_REGISTER_PID:
        return ScriptEnginePseudoRegGetPid();
    case PSEUDO_REGISTER_PNAME:
        return (UINT64)ScriptEnginePseudoRegGetPname();
    case PSEUDO_REGISTER_CORE:
        return ScriptEnginePseudoRegGetCore();
    case PSEUDO_REGISTER_PROC:
        return ScriptEnginePseudoRegGetProc();
    case PSEUDO_REGISTER_THREAD:
        return ScriptEnginePseudoRegGetThread();
    case PSEUDO_REGISTER_PEB:
        return ScriptEnginePseudoRegGetPeb();
    case PSEUDO_REGISTER_TEB:
        return ScriptEnginePseudoRegGetTeb();
    case PSEUDO_REGISTER_IP:
        return ScriptEnginePseudoRegGetIp();
    case PSEUDO_REGISTER_BUFFER:
        if (ActionBuffer->CurrentAction != NULL)
        {
            return ScriptEnginePseudoRegGetBuffer(
                (UINT64 *)ActionBuffer->CurrentAction);
        }
        else
        {
            return NULL;
        }
    case PSEUDO_REGISTER_CONTEXT:
        return ActionBuffer->Context;
    case INVALID:
#ifdef SCRIPT_ENGINE_USER_MODE
        ShowMessages("error in reading regesiter");
#endif // SCRIPT_ENGINE_USER_MODE
        return INVALID;
        // TODO: Add all the register
    }
}

UINT64
GetValue(PGUEST_REGS                   GuestRegs,
         PACTION_BUFFER                ActionBuffer,
         PSCRIPT_ENGINE_VARIABLES_LIST VariablesList,
         PSYMBOL                       Symbol,
         BOOLEAN                       ReturnReference)
{
    switch (Symbol->Type)
    {
    case SYMBOL_GLOBAL_ID_TYPE:

        if (ReturnReference)
            return ((UINT64)(&VariablesList->GlobalVariablesList[Symbol->Value]));
        else
            return VariablesList->GlobalVariablesList[Symbol->Value];

    case SYMBOL_LOCAL_ID_TYPE:

        if (ReturnReference)
            return ((UINT64)(&VariablesList->LocalVariablesList[Symbol->Value]));
        else
            return VariablesList->LocalVariablesList[Symbol->Value];

    case SYMBOL_NUM_TYPE:

        if (ReturnReference)
            return ((UINT64)&Symbol->Value);
        else
            return Symbol->Value;

    case SYMBOL_REGISTER_TYPE:

        if (ReturnReference)
            return NULL; // Not reasonable, you should not dereference a register!
        else
            return GetRegValue(GuestRegs, (REGS_ENUM)Symbol->Value);

    case SYMBOL_PSEUDO_REG_TYPE:

        if (ReturnReference)
            return NULL; // Not reasonable, you should not dereference a pseudo-register!
        else
            return GetPseudoRegValue(Symbol, ActionBuffer);

    case SYMBOL_TEMP_TYPE:

        if (ReturnReference)
            return ((UINT64)&VariablesList->TempList[Symbol->Value]);
        else
            return VariablesList->TempList[Symbol->Value];
    }
}

VOID
SetRegValue(PGUEST_REGS GuestRegs, PSYMBOL Symbol, UINT64 Value)
{
    switch (Symbol->Value)
    {
    case REGISTER_RAX:
        GuestRegs->rax = Value;

        break;

    case REGISTER_EAX:
        GuestRegs->rax = (GuestRegs->rax & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_AX:
        GuestRegs->rax = (GuestRegs->rax & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_AH:
        GuestRegs->rax = (GuestRegs->rax & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_AL:
        GuestRegs->rax = (GuestRegs->rax & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;

    case REGISTER_RCX:
        GuestRegs->rcx = Value;

        break;
    case REGISTER_ECX:
        GuestRegs->rcx = (GuestRegs->rcx & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_CX:
        GuestRegs->rcx = (GuestRegs->rcx & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_CH:
        GuestRegs->rcx = (GuestRegs->rcx & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_CL:
        GuestRegs->rcx = (GuestRegs->rcx & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_RDX:
        GuestRegs->rdx = Value;

        break;
    case REGISTER_EDX:
        GuestRegs->rdx = (GuestRegs->rdx & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_DX:
        GuestRegs->rdx = (GuestRegs->rdx & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_DH:
        GuestRegs->rdx = (GuestRegs->rdx & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_DL:
        GuestRegs->rdx = (GuestRegs->rdx & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_RBX:
        GuestRegs->rbx = Value;

        break;
    case REGISTER_EBX:
        GuestRegs->rbx = (GuestRegs->rbx & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_BX:
        GuestRegs->rbx = (GuestRegs->rbx & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_BH:
        GuestRegs->rbx = (GuestRegs->rbx & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_BL:
        GuestRegs->rbx = (GuestRegs->rbx & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_RSP:

#ifdef SCRIPT_ENGINE_USER_MODE
        GuestRegs->rsp = Value;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        GuestRegs->rsp = Value;
        SetGuestRSP(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_ESP:

#ifdef SCRIPT_ENGINE_USER_MODE
        GuestRegs->rsp = Value;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        GuestRegs->rsp = (GuestRegs->rsp & UPPER_32_BITS) | (Value & LOWER_32_BITS);
        SetGuestRSP(GuestRegs->rsp);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_SP:

#ifdef SCRIPT_ENGINE_USER_MODE
        GuestRegs->rsp = Value;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        GuestRegs->rsp = (GuestRegs->rsp & UPPER_48_BITS) | (Value & LOWER_16_BITS);
        SetGuestRSP(GuestRegs->rsp);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_SPL:

#ifdef SCRIPT_ENGINE_USER_MODE
        GuestRegs->rsp = Value;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        GuestRegs->rsp = (GuestRegs->rsp & UPPER_56_BITS) | (Value & LOWER_8_BITS);
        SetGuestRSP(GuestRegs->rsp);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_RBP:
        GuestRegs->rbp = Value;

        break;
    case REGISTER_EBP:
        GuestRegs->rbp = (GuestRegs->rbp & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_BP:
        GuestRegs->rbp = (GuestRegs->rbp & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_BPL:
        GuestRegs->rbp = (GuestRegs->rbp & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_RSI:
        GuestRegs->rsi = Value;

        break;
    case REGISTER_ESI:
        GuestRegs->rsi = (GuestRegs->rsi & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_SI:
        GuestRegs->rsi = (GuestRegs->rsi & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_SIL:
        GuestRegs->rsi = (GuestRegs->rsi & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_RDI:
        GuestRegs->rdi = Value;

        break;
    case REGISTER_EDI:
        GuestRegs->rdi = (GuestRegs->rdi & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_DI:
        GuestRegs->rdi = (GuestRegs->rdi & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_DIL:
        GuestRegs->rdi = (GuestRegs->rdi & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_R8:
        GuestRegs->r8 = Value;

        break;
    case REGISTER_R8D:
        GuestRegs->r8 = (GuestRegs->r8 & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_R8W:
        GuestRegs->r8 = (GuestRegs->r8 & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_R8H:
        GuestRegs->r8 = (GuestRegs->r8 & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_R8L:
        GuestRegs->r8 = (GuestRegs->r8 & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_R9:
        GuestRegs->r9 = Value;

        break;
    case REGISTER_R9D:
        GuestRegs->r9 = (GuestRegs->r9 & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_R9W:
        GuestRegs->r9 = (GuestRegs->r9 & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_R9H:
        GuestRegs->r9 = (GuestRegs->r9 & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_R9L:
        GuestRegs->r9 = (GuestRegs->r9 & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_R10:
        GuestRegs->r10 = Value;

        break;
    case REGISTER_R10D:
        GuestRegs->r10 = (GuestRegs->r10 & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_R10W:
        GuestRegs->r10 = (GuestRegs->r10 & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_R10H:
        GuestRegs->r10 = (GuestRegs->r10 & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_R10L:
        GuestRegs->r10 = (GuestRegs->r10 & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_R11:
        GuestRegs->r11 = Value;

        break;
    case REGISTER_R11D:
        GuestRegs->r11 = (GuestRegs->r11 & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_R11W:
        GuestRegs->r11 = (GuestRegs->r11 & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_R11H:
        GuestRegs->r11 = (GuestRegs->r11 & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_R11L:
        GuestRegs->r11 = (GuestRegs->r11 & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_R12:
        GuestRegs->r12 = Value;

        break;
    case REGISTER_R12D:
        GuestRegs->r12 = (GuestRegs->r12 & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_R12W:
        GuestRegs->r12 = (GuestRegs->r12 & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_R12H:
        GuestRegs->r12 = (GuestRegs->r12 & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_R12L:
        GuestRegs->r12 = (GuestRegs->r12 & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_R13:
        GuestRegs->r13 = Value;

        break;
    case REGISTER_R13D:
        GuestRegs->r13 = (GuestRegs->r13 & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_R13W:
        GuestRegs->r13 = (GuestRegs->r13 & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_R13H:
        GuestRegs->r13 = (GuestRegs->r13 & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_R13L:
        GuestRegs->r13 = (GuestRegs->r13 & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_R14:
        GuestRegs->r14 = Value;

        break;
    case REGISTER_R14D:
        GuestRegs->r14 = (GuestRegs->r14 & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_R14W:
        GuestRegs->r14 = (GuestRegs->r14 & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_R14H:
        GuestRegs->r14 = (GuestRegs->r14 & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_R14L:
        GuestRegs->r14 = (GuestRegs->r14 & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_R15:
        GuestRegs->r15 = Value;

        break;
    case REGISTER_R15D:
        GuestRegs->r15 = (GuestRegs->r15 & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_R15W:
        GuestRegs->r15 = (GuestRegs->r15 & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_R15H:
        GuestRegs->r15 = (GuestRegs->r15 & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_R15L:
        GuestRegs->r15 = (GuestRegs->r15 & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_DS:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        Value = Value & LOWER_16_BITS;
        SetGuestDsSel((PVMX_SEGMENT_SELECTOR)&Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_ES:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        Value = Value & LOWER_16_BITS;
        SetGuestEsSel((PVMX_SEGMENT_SELECTOR)&Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_FS:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        Value = Value & LOWER_16_BITS;
        SetGuestFsSel((PVMX_SEGMENT_SELECTOR)&Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_GS:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        Value = Value & LOWER_16_BITS;
        SetGuestGsSel((PVMX_SEGMENT_SELECTOR)&Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CS:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        Value = Value & LOWER_16_BITS;
        SetGuestCsSel((PVMX_SEGMENT_SELECTOR)&Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_SS:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        Value = Value & LOWER_16_BITS;
        SetGuestSsSel((PVMX_SEGMENT_SELECTOR)&Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_RFLAGS:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_EFLAGS:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags((GetGuestRFlags() & UPPER_32_BITS) | (Value & LOWER_32_BITS));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_FLAGS:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags((GetGuestRFlags() & UPPER_48_BITS) | (Value & LOWER_16_BITS));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_CF) : GetGuestRFlags() & (~(X86_FLAGS_CF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_PF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_PF) : GetGuestRFlags() & (~(X86_FLAGS_PF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_AF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_AF) : GetGuestRFlags() & (~(X86_FLAGS_AF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_ZF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_ZF) : GetGuestRFlags() & (~(X86_FLAGS_ZF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_SF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_SF) : GetGuestRFlags() & (~(X86_FLAGS_SF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_TF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_TF) : GetGuestRFlags() & (~(X86_FLAGS_TF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_IF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_IF) : GetGuestRFlags() & (~(X86_FLAGS_IF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_DF) : GetGuestRFlags() & (~(X86_FLAGS_DF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_OF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_OF) : GetGuestRFlags() & (~(X86_FLAGS_OF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_IOPL:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (1 << X86_FLAGS_IOPL_SHIFT) : GetGuestRFlags() & (~(1 << X86_FLAGS_IOPL_SHIFT)));
        Value = (Value >> 4) & 1;
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (1 << X86_FLAGS_IOPL_SHIFT_2ND_BIT) : GetGuestRFlags() & (~(1 << X86_FLAGS_IOPL_SHIFT_2ND_BIT)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_NT:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_NT) : GetGuestRFlags() & (~(X86_FLAGS_NT)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_RF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_RF) : GetGuestRFlags() & (~(X86_FLAGS_RF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_VM:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_VM) : GetGuestRFlags() & (~(X86_FLAGS_VM)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_AC:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_AC) : GetGuestRFlags() & (~(X86_FLAGS_AC)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_VIF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_VIF) : GetGuestRFlags() & (~(X86_FLAGS_VIF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_VIP:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_VIP) : GetGuestRFlags() & (~(X86_FLAGS_VIP)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_ID:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_ID) : GetGuestRFlags() & (~(X86_FLAGS_ID)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_RIP:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRIP(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_EIP:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRIP((GetGuestRIP() & UPPER_32_BITS) | (Value & LOWER_32_BITS));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_IP:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRIP((GetGuestRIP() & UPPER_48_BITS) | (Value & LOWER_16_BITS));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_IDTR:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestIdtr(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_LDTR:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestLdtr(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_GDTR:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestGdtr(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_TR:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestTr(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CR0:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestCr0(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CR2:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestCr2(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CR3:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestCr3(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CR4:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestCr4(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CR8:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestCr8(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR0:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestDr0(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR1:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestDr1(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR2:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestDr2(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR3:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestDr3(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR6:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestDr6(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR7:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestDr7(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;
    }
}

VOID
SetValue(PGUEST_REGS GuestRegs, SCRIPT_ENGINE_VARIABLES_LIST * VariablesList, PSYMBOL Symbol, UINT64 Value)
{
    switch (Symbol->Type)
    {
    case SYMBOL_GLOBAL_ID_TYPE:
        VariablesList->GlobalVariablesList[Symbol->Value] = Value;
        return;
    case SYMBOL_LOCAL_ID_TYPE:
        VariablesList->LocalVariablesList[Symbol->Value] = Value;
        return;
    case SYMBOL_TEMP_TYPE:
        VariablesList->TempList[Symbol->Value] = Value;
        return;
    case SYMBOL_REGISTER_TYPE:
        SetRegValue(GuestRegs, Symbol, Value);
        return;
    }
}

VOID
ScriptEngineGetOperatorName(PSYMBOL OperatorSymbol, CHAR * BufferForName)
{
    switch (OperatorSymbol->Value)
    {
    case FUNC_POI:
        memcpy(BufferForName, "poi", 3);
        break;
    case FUNC_DB:
        memcpy(BufferForName, "db", 2);
        break;
    case FUNC_DD:
        memcpy(BufferForName, "dd", 2);
        break;
    case FUNC_DW:
        memcpy(BufferForName, "dw", 2);
        break;
    case FUNC_DQ:
        memcpy(BufferForName, "dq", 2);
        break;
    case FUNC_HI:
        memcpy(BufferForName, "hi", 2);
        break;
    case FUNC_LOW:
        memcpy(BufferForName, "low", 3);
        break;
    default:
        memcpy(BufferForName, "error", 5);
        break;
    }
}

BOOL
ScriptEngineExecute(PGUEST_REGS                    GuestRegs,
                    ACTION_BUFFER *                ActionDetail,
                    SCRIPT_ENGINE_VARIABLES_LIST * VariablesList,
                    SYMBOL_BUFFER *                CodeBuffer,
                    int *                          Indx,
                    SYMBOL *                       ErrorOperator)
{
    PSYMBOL Operator;
    PSYMBOL Src0;
    PSYMBOL Src1;
    PSYMBOL Src2;

    PSYMBOL Des;
    UINT64  SrcVal0;
    UINT64  SrcVal1;
    UINT64  SrcVal2;

    UINT64 DesVal;
    BOOL   HasError = FALSE;

    Operator = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

    *ErrorOperator = *Operator;

    *Indx = *Indx + 1;

    if (Operator->Type != SYMBOL_SEMANTIC_RULE_TYPE)
    {
#ifdef SCRIPT_ENGINE_USER_MODE
        ShowMessages("err, expecting operator type\n");
        return HasError;
#endif // SCRIPT_ENGINE_USER_MODE
    };

    switch (Operator->Value)
    {
    case FUNC_ED:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEd(SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_EB:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEb(SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_EQ:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionEq(SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_INTERLOCKED_EXCHANGE:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedExchange((volatile long long *)SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_INTERLOCKED_EXCHANGE_ADD:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedExchangeAdd((volatile long long *)SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_INTERLOCKED_COMPARE_EXCHANGE:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal2 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src2, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedCompareExchange((volatile long long *)SrcVal2, SrcVal1, SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_SPINLOCK_LOCK_CUSTOM_WAIT:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        ScriptEngineFunctionSpinlockLockCustomWait((volatile long *)SrcVal1, SrcVal0, &HasError);

        return HasError;

    case FUNC_PAUSE:

        ScriptEngineFunctionPause(ActionDetail->Tag,
                                  ActionDetail->ImmediatelySendTheResults,
                                  GuestRegs,
                                  ActionDetail->Context);
        return HasError;

    case FUNC_FLUSH:

        ScriptEngineFunctionFlush();

        return HasError;

    case FUNC_EVENT_IGNORE:

        ScriptEngineFunctionEventIgnore();

        return HasError;

    case FUNC_OR:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = SrcVal1 | SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_INC:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        DesVal = SrcVal0 + 1;

        Des = Src0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_DEC:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        DesVal = SrcVal0 - 1;

        Des = Src0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_XOR:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 ^ SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_AND:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 & SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_ASR:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 >> SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_ASL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = SrcVal1 << SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_ADD:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 + SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_SUB:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 - SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_MUL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 * SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_DIV:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        if (SrcVal0 == 0)
        {
            HasError = TRUE;
            return HasError;
        }

        DesVal = SrcVal1 / SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_MOD:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        if (SrcVal0 == 0)
        {
            HasError = TRUE;
            return HasError;
        }

        DesVal = SrcVal1 % SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_GT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 > SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_LT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 < SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_EGT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 >= SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_ELT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = SrcVal1 <= SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_EQUAL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 == SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_NEQ:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal1 != SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_POI:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordPoi((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE),
                                        &HasError);
        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_DB:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDb((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE),
                                       &HasError);
        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_DD:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDd((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE),
                                       &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_DW:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDw((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE),
                                       &HasError);
        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_DQ:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordDq((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE),
                                       &HasError);
        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_NOT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ~SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_REFERENCE:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        //
        // It's reference, we need an address
        //
        SrcVal0 = GetValue(GuestRegs, ActionDetail, VariablesList, Src0, TRUE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_PHYSICAL_TO_VIRTUAL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionPhysicalToVirtual(GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE));

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_VIRTUAL_TO_PHYSICAL:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionVirtualToPhysical(GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE));

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_CHECK_ADDRESS:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        if (ScriptEngineCheckAddressValidity(SrcVal0, sizeof(BYTE)))
            DesVal = 1; // TRUE
        else
            DesVal = 0; // FALSE

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_STRLEN:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionStrlen((const char *)SrcVal0);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_WCSLEN:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionWcslen((const wchar_t *)SrcVal0);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_INTERLOCKED_INCREMENT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedIncrement((volatile long long *)SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_INTERLOCKED_DECREMENT:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineFunctionInterlockedDecrement((volatile long long *)SrcVal0, &HasError);

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_NEG:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = -(INT64)SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_HI:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordHi((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE),
                                       &HasError);
        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_LOW:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        DesVal = ScriptEngineKeywordLow((PUINT64)GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE),
                                        &HasError);
        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_MOV:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Des   = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                        (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        DesVal = SrcVal0;

        SetValue(GuestRegs, VariablesList, Des, DesVal);

        return HasError;

    case FUNC_PRINT:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        //
        // Call the target function
        //
        ScriptEngineFunctionPrint(ActionDetail->Tag,
                                  ActionDetail->ImmediatelySendTheResults,
                                  SrcVal0);
        return HasError;

    case FUNC_TEST_STATEMENT:
        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        //
        // Call the target function
        //
        ScriptEngineFunctionTestStatement(ActionDetail->Tag,
                                          ActionDetail->ImmediatelySendTheResults,
                                          SrcVal0);
        return HasError;

    case FUNC_SPINLOCK_LOCK:
        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        //
        // Call the target function
        //
        ScriptEngineFunctionSpinlockLock((volatile LONG *)SrcVal0, &HasError);

        return HasError;

    case FUNC_SPINLOCK_UNLOCK:
        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        //
        // Call the target function
        //
        ScriptEngineFunctionSpinlockUnlock((volatile LONG *)SrcVal0, &HasError);

        return HasError;

    case FUNC_EVENT_DISABLE:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        ScriptEngineFunctionDisableEvent(
            ActionDetail->Tag,
            ActionDetail->ImmediatelySendTheResults,
            SrcVal0);

        return HasError;

    case FUNC_EVENT_ENABLE:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        ScriptEngineFunctionEnableEvent(
            ActionDetail->Tag,
            ActionDetail->ImmediatelySendTheResults,
            SrcVal0);

        return HasError;

    case FUNC_FORMATS:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        //
        // Call the target function
        //
        ScriptEngineFunctionFormats(
            ActionDetail->Tag,
            ActionDetail->ImmediatelySendTheResults,
            SrcVal0);

        return HasError;

    case FUNC_JZ:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        if (SrcVal1 == 0)
            *Indx = SrcVal0;

        return HasError;

    case FUNC_JNZ:

        Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;
        SrcVal1 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src1, FALSE);

        if (SrcVal1 != 0)
            *Indx = SrcVal0;

        return HasError;

    case FUNC_JMP:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;
        SrcVal0 =
            GetValue(GuestRegs, ActionDetail, VariablesList, Src0, FALSE);

        *Indx = SrcVal0;

        return HasError;

    case FUNC_PRINTF:

        Src0  = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));
        *Indx = *Indx + 1;

        //
        // Call the target function
        //

        *Indx =
            *Indx + ((sizeof(unsigned long long) + strlen((char *)&Src0->Value)) /
                     sizeof(SYMBOL));

        Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                         (unsigned long long)(*Indx * sizeof(SYMBOL)));

        *Indx = *Indx + 1;

        PSYMBOL Src2 = NULL;

        if (Src1->Value > 0)
        {
            Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                             (unsigned long long)(*Indx * sizeof(SYMBOL)));

            *Indx = *Indx + Src1->Value;
        }

        ScriptEngineFunctionPrintf(
            GuestRegs,
            ActionDetail,
            VariablesList,
            ActionDetail->Tag,
            ActionDetail->ImmediatelySendTheResults,
            (char *)&Src0->Value,
            Src1->Value,
            Src2,
            (BOOLEAN *)&HasError);

        return HasError;
    }
}
