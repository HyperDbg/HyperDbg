/**
 * @file ScriptEngineCommon.h
 * @author M.H. Gholamrezei (gholamrezaei.mh@gmail.com)
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Shared Headers for Script engine
 * @details
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once
#include "ScriptEngineCommonDefinitions.h"

#ifndef PacketChunkSize
#define PacketChunkSize 1000
#endif // !PacketChunkSize

//
// Wrapper headers
//
#ifdef SCRIPT_ENGINE_KERNEL_MODE

UINT64
ScriptEngineWrapperGetInstructionPointer();

UINT64
ScriptEngineWrapperGetAddressOfReservedBuffer(PDEBUGGER_EVENT_ACTION Action);

BOOLEAN
CheckMemoryAccessSafety(UINT64 TargetAddress, UINT32 Size);

UINT32
VmxrootCompatibleStrlen(const CHAR *S);

BOOLEAN
MemoryMapperReadMemorySafe(UINT64 VaAddressToRead, PVOID BufferToSaveMemory,
                           SIZE_T SizeToRead);

#endif // SCRIPT_ENGINE_KERNEL_MODE

typedef unsigned long long QWORD;
typedef unsigned __int64 UINT64, *PUINT64;
typedef unsigned long DWORD;
typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef int INT;
typedef unsigned int UINT;
typedef unsigned int *PUINT;
typedef unsigned __int64 ULONG64, *PULONG64;
typedef unsigned __int64 DWORD64, *PDWORD64;
#define VOID void
typedef char CHAR;
typedef wchar_t WCHAR;

typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned long ULONG;

typedef UCHAR BOOLEAN;     // winnt
typedef BOOLEAN *PBOOLEAN; // winnt

typedef signed char INT8, *PINT8;
typedef signed short INT16, *PINT16;
typedef signed int INT32, *PINT32;
typedef signed __int64 INT64, *PINT64;
typedef unsigned char UINT8, *PUINT8;
typedef unsigned short UINT16, *PUINT16;
typedef unsigned int UINT32, *PUINT32;
typedef unsigned __int64 UINT64, *PUINT64;

#define FALSE 0
#define TRUE 1

typedef struct _GUEST_REGS_USER_MODE_USER_MODE {
  ULONG64 rax; // 0x00
  ULONG64 rcx; // 0x08
  ULONG64 rdx; // 0x10
  ULONG64 rbx; // 0x18
  ULONG64 rsp; // 0x20
  ULONG64 rbp; // 0x28
  ULONG64 rsi; // 0x30
  ULONG64 rdi; // 0x38
  ULONG64 r8;  // 0x40
  ULONG64 r9;  // 0x48
  ULONG64 r10; // 0x50
  ULONG64 r11; // 0x58
  ULONG64 r12; // 0x60
  ULONG64 r13; // 0x68
  ULONG64 r14; // 0x70
  ULONG64 r15; // 0x78
} GUEST_REGS_USER_MODE, *PGUEST_REGS_USER_MODE;

#define LOWORD(l) ((WORD)(l))
#define HIWORD(l) ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w) ((BYTE)(w))
#define HIBYTE(w) ((BYTE)(((WORD)(w) >> 8) & 0xFF))

#define MAX_TEMP_COUNT 32

// TODO: Extract number of variables from input of ScriptEngine
// and allocate variableList Dynamically.
#define MAX_VAR_COUNT 32

#define MAX_FUNCTION_NAME_LENGTH 32

//////////////////////////////////////////////////
//            	     Imports                    //
//////////////////////////////////////////////////

#ifdef SCRIPT_ENGINE_USER_MODE
extern "C" {
__declspec(dllimport) PSYMBOL_BUFFER ScriptEngineParse(char *str);
__declspec(dllimport) void PrintSymbolBuffer(const PSYMBOL_BUFFER SymbolBuffer);
__declspec(dllimport) void PrintSymbol(PSYMBOL Symbol);
__declspec(dllimport) void RemoveSymbolBuffer(PSYMBOL_BUFFER SymbolBuffer);
}
#endif // SCRIPT_ENGINE_USER_MODE

UINT64 GetValue(PGUEST_REGS_USER_MODE GuestRegs, ACTION_BUFFER ActionBuffer,
                UINT64 *g_TempList, UINT64 *g_VariableList, PSYMBOL Symbol);

//
// Pseudo registers
//
// $tid
UINT64 ScriptEnginePseudoRegGetTid() {

#ifdef SCRIPT_ENGINE_USER_MODE
  return GetCurrentThreadId();
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
  return PsGetCurrentThreadId();
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

// $pid
UINT64 ScriptEnginePseudoRegGetPid() {

#ifdef SCRIPT_ENGINE_USER_MODE
  return GetCurrentProcessId();
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
  return PsGetCurrentProcessId();
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

// $proc
UINT64 ScriptEnginePseudoRegGetProc() {

#ifdef SCRIPT_ENGINE_USER_MODE
  return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
  return PsGetCurrentProcess();
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

// $thread
UINT64 ScriptEnginePseudoRegGetThread() {

#ifdef SCRIPT_ENGINE_USER_MODE
  return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
  return PsGetCurrentThread();
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

// $peb
UINT64 ScriptEnginePseudoRegGetPeb() {

#ifdef SCRIPT_ENGINE_USER_MODE
  //
  // Hand-rolled structs ( may cause conflict depending on your dev env )
  //
  struct PROCESS_BASIC_INFORMATION {
    PVOID Reserved1;
    PVOID PebBaseAddress;
    PVOID Reserved2[2];
    ULONG_PTR UniqueProcessId;
    PVOID Reserved3;
  };

  struct PEB_LDR_DATA {
    BYTE Reserved1[8];
    PVOID Reserved2[3];
    LIST_ENTRY InMemoryOrderModuleList;
  };

  struct PEB {
    BYTE Reserved1[2];
    BYTE BeingDebugged;
    BYTE Reserved2[1];
    PVOID Reserved3[2];
    struct PEB_LDR_DATA *Ldr;
    PVOID ProcessParameters; /* PRTL_USER_PROCESS_PARAMETERS */
    BYTE Reserved4[104];
    PVOID Reserved5[52];
    PVOID PostProcessInitRoutine; /* PPS_POST_PROCESS_INIT_ROUTINE */
    BYTE Reserved6[128];
    PVOID Reserved7[1];
    ULONG SessionId;
  };

  struct UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
  };

  struct LDR_MODULE {
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
    PVOID BaseAddress;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    struct UNICODE_STRING FullDllName;
    struct UNICODE_STRING BaseDllName;
    ULONG Flags;
    SHORT LoadCount;
    SHORT TlsIndex;
    LIST_ENTRY HashTableEntry;
    ULONG TimeDateStamp;
  };

  enum PROCESSINFOCLASS {
    ProcessBasicInformation = 0,
    ProcessDebugPort = 7,
    ProcessWow64Information = 26,
    ProcessImageFileName = 27
  };

  LPCWSTR NTDLL_NAME = L"ntdll.dll";
  LPCSTR NTQUERYINFO_NAME = "NtQueryInformationProcess";

  HMODULE NtdllMod;
  HANDLE ThisProcess;
  NTSTATUS NtCallRet;
  ULONG BytesReturned;

  //
  // function pointer to house result from GetProcAddress
  //
  NTSTATUS(WINAPI * QueryInfoProcPtr)
  (HANDLE, enum PROCESSINFOCLASS, PVOID, ULONG, PULONG);

  struct PROCESS_BASIC_INFORMATION BasicInfo;
  struct PEB *PebPtr;
  struct LDR_MODULE *modPtr;

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
      (NTSTATUS(WINAPI *)(HANDLE, enum PROCESSINFOCLASS, PVOID, ULONG,
                          PULONG))GetProcAddress(NtdllMod, NTQUERYINFO_NAME);

  //
  // call function on self; introspect
  //
  NtCallRet = QueryInfoProcPtr(ThisProcess, ProcessBasicInformation, &BasicInfo,
                               sizeof(BasicInfo), &BytesReturned);

  //
  // get peb ptr and decode some if its fields
  //
  PebPtr = (struct PEB *)BasicInfo.PebBaseAddress;

  /* ShowMessages("PEB : %p\n", PebPtr); */

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
UINT64 ScriptEnginePseudoRegGetTeb() {

#ifdef SCRIPT_ENGINE_USER_MODE
  return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
  return PsGetCurrentThreadTeb();
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

// $ip
UINT64 ScriptEnginePseudoRegGetIp() {

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
UINT64 ScriptEnginePseudoRegGetBuffer(UINT64 *CorrespondingAction) {

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
BOOLEAN ScriptEngineCheckAddressValidity(PUINT64 Address, UINT32 Length) {

#ifdef SCRIPT_ENGINE_USER_MODE

  //
  // Actually, there is no way to check this validity as it causes cpu
  // errors, so the only solution is using SEH which is stupid idea,
  // sure we don't want to create SEH frame each time we need to check
  // a function address, so I don't know what to do, let's return TRUE
  // for now
  //
  return TRUE;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

  if (VirtualAddressToPhysicalAddress(Address) == 0) {
    return FALSE;
  } else {
    if (VirtualAddressToPhysicalAddress(Address + Length) == 0) {
      return FALSE;
    } else {
      return TRUE;
    }
  }
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

//
// Keywords
//

// poi
UINT64 ScriptEngineKeywordPoi(PUINT64 Address, BOOL *HasError) {

#ifdef SCRIPT_ENGINE_KERNEL_MODE

  if (!CheckMemoryAccessSafety(Address, sizeof(UINT64))) {
    *HasError = TRUE;

    return NULL;
  }

#endif // SCRIPT_ENGINE_KERNEL_MODE

  return *Address;
}

// hi
WORD ScriptEngineKeywordHi(PUINT64 Address, BOOL *HasError) {

#ifdef SCRIPT_ENGINE_KERNEL_MODE

  if (!CheckMemoryAccessSafety(Address, sizeof(UINT64))) {
    *HasError = TRUE;

    return NULL;
  }

#endif // SCRIPT_ENGINE_KERNEL_MODE

  QWORD Result = *Address;
  return HIWORD(Result);
}

// low
WORD ScriptEngineKeywordLow(PUINT64 Address, BOOL *HasError) {

#ifdef SCRIPT_ENGINE_KERNEL_MODE

  if (!CheckMemoryAccessSafety(Address, sizeof(UINT64))) {
    *HasError = TRUE;

    return NULL;
  }

#endif // SCRIPT_ENGINE_KERNEL_MODE

  QWORD Result = *Address;
  return LOWORD(Result);
}

// db
BYTE ScriptEngineKeywordDb(PUINT64 Address, BOOL *HasError) {

#ifdef SCRIPT_ENGINE_KERNEL_MODE

  if (!CheckMemoryAccessSafety(Address, sizeof(UINT64))) {
    *HasError = TRUE;

    return NULL;
  }

#endif // SCRIPT_ENGINE_KERNEL_MODE

  BYTE Result = *Address;
  return Result;
}

// dd
WORD ScriptEngineKeywordDd(PUINT64 Address, BOOL *HasError) {

#ifdef SCRIPT_ENGINE_KERNEL_MODE

  if (!CheckMemoryAccessSafety(Address, sizeof(UINT64))) {
    *HasError = TRUE;

    return NULL;
  }

#endif // SCRIPT_ENGINE_KERNEL_MODE

  WORD Result = *Address;
  return Result;
}

// dw
DWORD ScriptEngineKeywordDw(PUINT64 Address, BOOL *HasError) {

#ifdef SCRIPT_ENGINE_KERNEL_MODE

  if (!CheckMemoryAccessSafety(Address, sizeof(UINT64))) {
    *HasError = TRUE;

    return NULL;
  }

#endif // SCRIPT_ENGINE_KERNEL_MODE

  DWORD Result = *Address;
  return Result;
}

// dq
QWORD ScriptEngineKeywordDq(PUINT64 Address, BOOL *HasError) {

#ifdef SCRIPT_ENGINE_KERNEL_MODE

  if (!CheckMemoryAccessSafety(Address, sizeof(UINT64))) {
    *HasError = TRUE;

    return NULL;
  }

#endif // SCRIPT_ENGINE_KERNEL_MODE

  QWORD Result = *Address;
  return Result;
}

// str
CHAR *ScriptEngineKeywordStr(CHAR *Address, BOOL *HasError) {

#ifdef SCRIPT_ENGINE_KERNEL_MODE

  if (!CheckMemoryAccessSafety(Address, sizeof(UINT64))) {
    *HasError = TRUE;

    return NULL;
  }

#endif // SCRIPT_ENGINE_KERNEL_MODE

  UINT64 Len = 0;

  Len = strlen(Address) + 1;

  if (Len == 1) {
    return NULL;
  } else {
    return Address;
  }
}

// wstr
WCHAR *ScriptEngineKeywordWstr(WCHAR *Address, BOOL *HasError) {

#ifdef SCRIPT_ENGINE_KERNEL_MODE

  if (!CheckMemoryAccessSafety(Address, sizeof(UINT64))) {
    *HasError = TRUE;

    return NULL;
  }

#endif // SCRIPT_ENGINE_KERNEL_MODE

  UINT64 Len = 0;

  Len = wcslen(Address) + 1;

  if (Len == 1) {
    return NULL;
  } else {
    return Address;
  }
}

//
// Functions
//
VOID ScriptEngineFunctionPrint(UINT64 Tag, BOOLEAN ImmediateMessagePassing,
                               UINT64 Value) {

#ifdef SCRIPT_ENGINE_USER_MODE
  ShowMessages("%llx\n", Value);

#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
  LogSimpleWithTag(Tag, ImmediateMessagePassing, "%llx\n", Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

VOID ScriptEngineFunctionDisableEvent(UINT64 Tag,
                                      BOOLEAN ImmediateMessagePassing,
                                      UINT64 Value) {

#ifdef SCRIPT_ENGINE_USER_MODE
  ShowMessages("err, disabling events is not possible in user-mode.\n");
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
  if (!DebuggerDisableEvent(Value + DebuggerEventTagStartSeed)) {
    LogInfo("Invalid tag id (%d).", Value);
  }
#endif // SCRIPT_ENGINE_KERNEL_MODE
}
VOID ScriptEngineFunctionEnableEvent(UINT64 Tag,
                                     BOOLEAN ImmediateMessagePassing,
                                     UINT64 Value) {

#ifdef SCRIPT_ENGINE_USER_MODE
  ShowMessages("err, enabling events is not possible in user-mode.\n");
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
  if (!DebuggerEnableEvent(Value + DebuggerEventTagStartSeed)) {
    LogInfo("Invalid tag id (%d).", Value);
  }
#endif // SCRIPT_ENGINE_KERNEL_MODE
}
VOID ScriptEngineFunctionBreak(UINT64 Tag, BOOLEAN ImmediateMessagePassing,
                               PGUEST_REGS_USER_MODE GuestRegs,
                               UINT64 Context) {

#ifdef SCRIPT_ENGINE_USER_MODE
  ShowMessages("err, breaking is not possible in user-mode.\n");
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

  DEBUGGER_TRIGGERED_EVENT_DETAILS ContextAndTag = {0};
  UINT32 CurrentProcessorIndex = KeGetCurrentProcessorNumber();

  if (g_GuestState[CurrentProcessorIndex].IsOnVmxRootMode) {
    //
    // The guest is already in vmx-root mode
    // Halt other cores
    //
    ContextAndTag.Tag = Tag;
    ContextAndTag.Context = Context;

    KdHandleBreakpointAndDebugBreakpoints(
        CurrentProcessorIndex, GuestRegs,
        DEBUGGEE_PAUSING_REASON_DEBUGGEE_EVENT_TRIGGERED, &ContextAndTag);

  } else {
    //
    // The guest is on vmx non-root mode
    //
    AsmVmxVmcall(VMCALL_VM_EXIT_HALT_SYSTEM, 0, 0, 0);
  }
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

VOID ScriptEngineFunctionFormats(UINT64 Tag, BOOLEAN ImmediateMessagePassing,
                                 UINT64 Value) {

#ifdef SCRIPT_ENGINE_USER_MODE
  ShowMessages("%llx\n", Value);

#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
  if (g_KernelDebuggerState) {
    KdSendFormatsFunctionResult(Value);
  } else {
    LogSimpleWithTag(Tag, ImmediateMessagePassing, "%llx\n", Value);
  }
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

VOID ScriptEngineFunctionJson(UINT64 Tag, BOOLEAN ImmediateMessagePassing,
                              char *Name, UINT64 Value) {

#ifdef SCRIPT_ENGINE_USER_MODE
  ShowMessages("%s : %d\n", Name, Value);
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
  // LogSimpleWithTag(Tag, ImmediateMessagePassing, "%s : %d\n", Name, Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

UINT32 CustomStrlen(UINT64 StrAddr, BOOLEAN IsWstring) {

#ifdef SCRIPT_ENGINE_USER_MODE
  return strlen((const char *)StrAddr);
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
  return VmxrootCompatibleStrlen((const CHAR *)StrAddr);
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

BOOLEAN CheckIfStringIsSafe(UINT64 StrAddr, BOOLEAN IsWstring) {

#ifdef SCRIPT_ENGINE_USER_MODE
  return TRUE;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

  //
  // At least two chars (wchar_t is 4 byte
  //
  if (CheckMemoryAccessSafety(StrAddr, IsWstring ? 4 : 2)) {
    return TRUE;
  } else {
    return FALSE;
  }

#endif // SCRIPT_ENGINE_KERNEL_MODE
}

VOID ApplyFormatSpecifier(const CHAR *CurrentSpecifier, CHAR *FinalBuffer,
                          PUINT32 CurrentProcessedPositionFromStartOfFormat,
                          PUINT32 CurrentPositionInFinalBuffer, UINT64 Val,
                          UINT32 SizeOfFinalBuffer) {

  UINT32 TempBufferLen = 0;
  CHAR TempBuffer[50 + 1] = {
      0}; // Maximum uint64_t is 18446744073709551615 + 1 thus its 20 character
          // for maximum buffer + 1 end char null but we alloc 50 to be sure

  *CurrentProcessedPositionFromStartOfFormat =
      *CurrentProcessedPositionFromStartOfFormat + strlen(CurrentSpecifier);
  sprintf(TempBuffer, CurrentSpecifier, Val);
  TempBufferLen = strlen(TempBuffer);

  //
  // Check final buffer capacity
  //
  if (*CurrentPositionInFinalBuffer + TempBufferLen > SizeOfFinalBuffer) {

    //
    // Over passed buffer
    //
    return;
  }

  memcpy(&FinalBuffer[*CurrentPositionInFinalBuffer], TempBuffer,
         TempBufferLen);

  *CurrentPositionInFinalBuffer = *CurrentPositionInFinalBuffer + TempBufferLen;
}

BOOLEAN
ApplyStringFormatSpecifier(const CHAR *CurrentSpecifier, CHAR *FinalBuffer,
                           PUINT32 CurrentProcessedPositionFromStartOfFormat,
                           PUINT32 CurrentPositionInFinalBuffer, UINT64 Val,
                           BOOLEAN IsWstring, UINT32 SizeOfFinalBuffer) {
  UINT32 StringSize;

  //
  // First we have to check if string is valid or not
  //
  if (!CheckIfStringIsSafe(Val, IsWstring)) {
    return FALSE;
  }
  StringSize = CustomStrlen(Val, IsWstring);

  //
  // get the length of the string (format) identifier
  //
  *CurrentProcessedPositionFromStartOfFormat += strlen(CurrentSpecifier);

  //
  // Check final buffer capacity
  //
  if (*CurrentPositionInFinalBuffer + StringSize > SizeOfFinalBuffer) {

    //
    // Over passed buffer
    //
    return TRUE;
  }

  //
  // Move the buffer string into the target buffer
  //
#ifdef SCRIPT_ENGINE_USER_MODE
  memcpy(&FinalBuffer[*CurrentPositionInFinalBuffer], (void *)Val, StringSize);
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
  MemoryMapperReadMemorySafe(Val, &FinalBuffer[*CurrentPositionInFinalBuffer],
                             StringSize);
#endif // SCRIPT_ENGINE_KERNEL_MODE

  *CurrentPositionInFinalBuffer += StringSize;

  return TRUE;
}

VOID ScriptEngineFunctionPrintf(PGUEST_REGS_USER_MODE GuestRegs,
                                ACTION_BUFFER ActionDetail, UINT64 *g_TempList,
                                UINT64 *g_VariableList, UINT64 Tag,
                                BOOLEAN ImmediateMessagePassing, char *Format,
                                UINT64 ArgCount, PSYMBOL FirstArg,
                                BOOLEAN *HasError) {

  *HasError = FALSE;
  PSYMBOL Symbol;

  UINT32 i = 0;

  char *Str = Format;

  do {

    //
    // Not the best way but some how for optimization
    //
    if (*Str == '%') {

      CHAR Temp = *(Str + 1);

      if (Temp == 'd' || Temp == 'i' || Temp == 'u' || Temp == 'o' ||
          Temp == 'x' || Temp == 'c' || Temp == 'p' || Temp == 's' ||

          !strncmp(Str, "%ws", 3) || !strncmp(Str, "%ls", 3) ||
          !strncmp(Str, "%lc", 3) ||

          !strncmp(Str, "%ld", 3) || !strncmp(Str, "%li", 3) ||
          !strncmp(Str, "%lu", 3) || !strncmp(Str, "%lo", 3) ||
          !strncmp(Str, "%lx", 3) ||

          !strncmp(Str, "%hd", 3) || !strncmp(Str, "%hi", 3) ||
          !strncmp(Str, "%hu", 3) || !strncmp(Str, "%ho", 3) ||
          !strncmp(Str, "%hx", 3) ||

          !strncmp(Str, "%lld", 4) || !strncmp(Str, "%lli", 4) ||
          !strncmp(Str, "%llu", 4) || !strncmp(Str, "%llo", 4) ||
          !strncmp(Str, "%llx", 4)

      ) {

        if (i < ArgCount)
          Symbol = FirstArg + i;
        else {
          *HasError = TRUE;
          break;
        }
        Symbol->Type &= 0xffffffff;
        Symbol->Type |= (UINT64)(Str - Format - 1) << 32;
        i++;
      }
    }
    Str++;
  } while (*Str);

  if (*HasError == FALSE)
    *HasError = (i != ArgCount);
  if (*HasError)
    return;

    //
    // Call printf
    //

    //
    // When we're here, all the pointers are the pointers including %ws and %s
    // pointers are checked and are safe to access
    //

#ifdef SCRIPT_ENGINE_USER_MODE
  printf("============================================================\n");
#endif // SCRIPT_ENGINE_USER_MODE

  char FinalBuffer[PacketChunkSize] = {0};
  UINT32 CurrentPositionInFinalBuffer = 0;
  UINT32 CurrentProcessedPositionFromStartOfFormat = 0;
  BOOLEAN WithoutAnyFormatSpecifier = TRUE;
  UINT64 Val;
  UINT32 Position;
  UINT32 LenOfFormats = strlen(Format) + 1;

  for (int i = 0; i < ArgCount; i++) {

    WithoutAnyFormatSpecifier = FALSE;
    Symbol = FirstArg + i;

    //
    // Address is either wstring (%ws) or string (%s)
    //

    Position = (Symbol->Type >> 32) + 1;
    Symbol->Type &= 0x7fffffff;
    Val = GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Symbol);

    CHAR PercentageChar = Format[Position];

    /*
    printf("position = %d is %c%c \n", Position, PercentageChar,
           IndicatorChar1);
           */

    if (CurrentProcessedPositionFromStartOfFormat != Position) {

      //
      // There is some strings before this format specifier
      // we should move it to the buffer
      //
      UINT32 StringLen = Position - CurrentProcessedPositionFromStartOfFormat;

      //
      // Check final buffer capacity
      //
      if (CurrentPositionInFinalBuffer + StringLen < sizeof(FinalBuffer)) {
        memcpy(&FinalBuffer[CurrentPositionInFinalBuffer],
               &Format[CurrentProcessedPositionFromStartOfFormat], StringLen);

        CurrentProcessedPositionFromStartOfFormat += StringLen;
        CurrentPositionInFinalBuffer += StringLen;
      }
    }

    //
    // Double check and apply
    //
    if (PercentageChar == '%') {

      //
      // Set first character of specifier
      //
      CHAR FormatSpecifier[5] = {0};
      FormatSpecifier[0] = '%';

      //
      // Read second char
      //
      CHAR IndicatorChar2 = Format[Position + 1];

      //
      // Check if IndicatorChar2 is 2 character long or more
      //
      if (IndicatorChar2 == 'l' || IndicatorChar2 == 'w' ||
          IndicatorChar2 == 'h') {

        //
        // Set second char in format specifier
        //
        FormatSpecifier[1] = IndicatorChar2;

        if (IndicatorChar2 == 'l' && Format[Position + 2] == 'l') {

          //
          // Set third character in format specifier "ll"
          //
          FormatSpecifier[2] = 'l';

          //
          // Set last character
          //
          FormatSpecifier[3] = Format[Position + 3];

        } else {

          //
          // Set last character
          //
          FormatSpecifier[2] = Format[Position + 2];
        }
      } else {
        //
        // It's a one char specifier (Set last character)
        //
        FormatSpecifier[1] = IndicatorChar2;
      }

      //
      // Apply the specifier
      //
      if (!strncmp(Str, "%s", 2)) {

        if (!ApplyStringFormatSpecifier(
                "%s", FinalBuffer, &CurrentProcessedPositionFromStartOfFormat,
                &CurrentPositionInFinalBuffer, Val, FALSE,
                sizeof(FinalBuffer))) {
          *HasError = TRUE;
          return;
        }
      } else if (!strncmp(Str, "%ls", 3) || !strncmp(Str, "%ws", 3)) {

        //
        // To be implemented for wstring
        //
      } else {
        ApplyFormatSpecifier(FormatSpecifier, FinalBuffer,
                             &CurrentProcessedPositionFromStartOfFormat,
                             &CurrentPositionInFinalBuffer, Val,
                             sizeof(FinalBuffer));
      }
    }
  }

  if (WithoutAnyFormatSpecifier) {
    //
    // Means that it's just a simple print without any format specifier
    //
    if (LenOfFormats < sizeof(FinalBuffer)) {
      memcpy(FinalBuffer, Format, LenOfFormats);
    }
  } else {

    //
    // Check if there is anything after the last format specifier
    //
    if (LenOfFormats > CurrentProcessedPositionFromStartOfFormat) {

      UINT32 RemainedLen =
          LenOfFormats - CurrentProcessedPositionFromStartOfFormat;

      if (CurrentPositionInFinalBuffer + RemainedLen < sizeof(FinalBuffer)) {
        memcpy(&FinalBuffer[CurrentPositionInFinalBuffer],
               &Format[CurrentProcessedPositionFromStartOfFormat], RemainedLen);
      }
    }
  }

//
// Print final result
//
#ifdef SCRIPT_ENGINE_USER_MODE
  printf("Final Buffer : %s\n", FinalBuffer);
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
  LogSimpleWithTag(Tag, ImmediateMessagePassing, "%s\n", FinalBuffer);
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

UINT64 GetRegValue(PGUEST_REGS_USER_MODE GuestRegs, PSYMBOL Symbol) {
  switch (Symbol->Value) {
  case REGISTER_RAX:
    return GuestRegs->rax;
  case REGISTER_RCX:
    return GuestRegs->rcx;
  case REGISTER_RDX:
    return GuestRegs->rdx;
  case REGISTER_RBX:
    return GuestRegs->rbx;
  case REGISTER_RSP:
    return GuestRegs->rsp;
  case REGISTER_RBP:
    return GuestRegs->rbp;
  case REGISTER_RSI:
    return GuestRegs->rsi;
  case REGISTER_RDI:
    return GuestRegs->rdi;
  case REGISTER_R8:
    return GuestRegs->r8;
  case REGISTER_R9:
    return GuestRegs->r9;
  case REGISTER_R10:
    return GuestRegs->r10;
  case REGISTER_R11:
    return GuestRegs->r11;
  case REGISTER_R12:
    return GuestRegs->r12;
  case REGISTER_R13:
    return GuestRegs->r13;
  case REGISTER_R14:
    return GuestRegs->r14;
  case REGISTER_R15:
    return GuestRegs->r15;
  case INVALID:
#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("Error in reading regesiter");
#endif // SCRIPT_ENGINE_USER_MODE
    return INVALID;
    // TODO: Add all the register
  }
}
UINT64 GetPseudoRegValue(PSYMBOL Symbol, ACTION_BUFFER ActionBuffer) {

  switch (Symbol->Value) {
  case PSEUDO_REGISTER_TID:
    return ScriptEnginePseudoRegGetTid();
  case PSEUDO_REGISTER_PID:
    return ScriptEnginePseudoRegGetPid();
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
    if (ActionBuffer.CurrentAction != NULL) {
      return ScriptEnginePseudoRegGetBuffer(
          (UINT64 *)ActionBuffer.CurrentAction);

    } else {
      return NULL;
    }
  case PSEUDO_REGISTER_CONTEXT:
    return ActionBuffer.Context;
  case INVALID:
#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("Error in reading regesiter");
#endif // SCRIPT_ENGINE_USER_MODE
    return INVALID;
    // TODO: Add all the register
  }
}

UINT64 GetValue(PGUEST_REGS_USER_MODE GuestRegs, ACTION_BUFFER ActionBuffer,
                UINT64 *g_TempList, UINT64 *g_VariableList, PSYMBOL Symbol) {

  switch (Symbol->Type) {
  case SYMBOL_ID_TYPE:
    return g_VariableList[Symbol->Value];
  case SYMBOL_NUM_TYPE:
    return Symbol->Value;
  case SYMBOL_REGISTER_TYPE:
    return GetRegValue(GuestRegs, Symbol);
  case SYMBOL_PSEUDO_REG_TYPE:
    return GetPseudoRegValue(Symbol, ActionBuffer);
  case SYMBOL_TEMP_TYPE:
    return g_TempList[Symbol->Value];
  }
}

VOID SetValue(PGUEST_REGS_USER_MODE GuestRegs, UINT64 *g_TempList,
              UINT64 *g_VariableList, PSYMBOL Symbol, UINT64 Value) {
  switch (Symbol->Type) {
  case SYMBOL_ID_TYPE:
    g_VariableList[Symbol->Value] = Value;
    return;
  case SYMBOL_TEMP_TYPE:
    g_TempList[Symbol->Value] = Value;
    return;
  }
}

VOID ScriptEngineGetOperatorName(PSYMBOL OperatorSymbol, CHAR *BufferForName) {

  switch (OperatorSymbol->Value) {
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
  case FUNC_STR:
    memcpy(BufferForName, "str", 3);
    break;
  case FUNC_WSTR:
    memcpy(BufferForName, "wstr", 4);
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

BOOL ScriptEngineExecute(PGUEST_REGS_USER_MODE GuestRegs,
                         ACTION_BUFFER ActionDetail, UINT64 *g_TempList,
                         UINT64 *g_VariableList, PSYMBOL_BUFFER CodeBuffer,
                         int *Indx, PSYMBOL ErrorOperator) {

  PSYMBOL Operator;
  PSYMBOL Src0;
  PSYMBOL Src1;
  PSYMBOL Des;
  UINT64 SrcVal0;
  UINT64 SrcVal1;
  UINT64 DesVal;
  BOOL HasError = FALSE;

  Operator = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                       (unsigned long long)(*Indx * sizeof(SYMBOL)));

  *ErrorOperator = *Operator;

  *Indx = *Indx + 1;
  if (Operator->Type != SYMBOL_SEMANTIC_RULE_TYPE) {

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("Error:Expecting Operator Type.\n");
    return HasError;
#endif // SCRIPT_ENGINE_USER_MODE
  }

  switch (Operator->Value) {

  case FUNC_BREAK:
    ScriptEngineFunctionBreak(ActionDetail.Tag,
                              ActionDetail.ImmediatelySendTheResults, GuestRegs,
                              ActionDetail.Context);
    return HasError;

  case FUNC_OR:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    SrcVal1 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal1 | SrcVal0;
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return HasError;

  case FUNC_XOR:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    SrcVal1 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal1 ^ SrcVal0;
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return HasError;

  case FUNC_AND:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    SrcVal1 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal1 & SrcVal0;
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return HasError;

  case FUNC_ASR:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    SrcVal1 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal1 >> SrcVal0;
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return HasError;

  case FUNC_ASL:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    SrcVal1 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal1 << SrcVal0;
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return HasError;

  case FUNC_ADD:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal1 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal1 + SrcVal0;
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return HasError;

  case FUNC_SUB:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal1 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal1 - SrcVal0;
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return HasError;
  case FUNC_MUL:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal1 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal1 * SrcVal0;
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return HasError;

  case FUNC_DIV:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal1 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal1 / SrcVal0;
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return HasError;
  case FUNC_MOD:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal1 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal1 % SrcVal0;
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return HasError;

  case FUNC_POI:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = ScriptEngineKeywordPoi((PUINT64)GetValue(GuestRegs, ActionDetail,
                                                      g_TempList,
                                                      g_VariableList, Src0),
                                    &HasError);
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return HasError;

  case FUNC_DB:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = ScriptEngineKeywordDb((PUINT64)GetValue(GuestRegs, ActionDetail,
                                                     g_TempList, g_VariableList,
                                                     Src0),
                                   &HasError);
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return HasError;
  case FUNC_DW:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = ScriptEngineKeywordDb((PUINT64)GetValue(GuestRegs, ActionDetail,
                                                     g_TempList, g_VariableList,
                                                     Src0),
                                   &HasError);
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return HasError;
  case FUNC_DQ:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = ScriptEngineKeywordDq((PUINT64)GetValue(GuestRegs, ActionDetail,
                                                     g_TempList, g_VariableList,
                                                     Src0),
                                   &HasError);
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return HasError;

  case FUNC_STR:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    // TODO: Hanlde str function

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("Error: STR functions is not handled yet.\n");
#endif // SCRIPT_ENGINE_USER_MODE

    return HasError;

  case FUNC_WSTR:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    // TODO: Hanlde wstr function

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("Error: WSTR functions is not handled yet.\n");
#endif // SCRIPT_ENGINE_USER_MODE

    return HasError;

  case FUNC_SIZEOF:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    // TODO: Hanlde sizeof function because we do not support pdb so
    // we don't have support this function
#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("Error: DB functions is not handled yet.\n");
#endif // SCRIPT_ENGINE_USER_MODE

    return HasError;

  case FUNC_NOT:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = ~SrcVal0;
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return HasError;

  case FUNC_NEG:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = -(INT64)SrcVal0;
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE
    return HasError;

  case FUNC_HI:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = ScriptEngineKeywordHi((PUINT64)GetValue(GuestRegs, ActionDetail,
                                                     g_TempList, g_VariableList,
                                                     Src0),
                                   &HasError);
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE
    return HasError;

  case FUNC_LOW:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = ScriptEngineKeywordLow((PUINT64)GetValue(GuestRegs, ActionDetail,
                                                      g_TempList,
                                                      g_VariableList, Src0),
                                    &HasError);
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE
    return HasError;
  case FUNC_MOV:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal0;
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);
    if (Des->Type == SYMBOL_ID_TYPE) {
#ifdef SCRIPT_ENGINE_USER_MODE
      ShowMessages("Result is %llx\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
      DbgBreakPoint();
      LogInfo("Result is %llx\n", DesVal);
#endif // SCRIPT_ENGINE_KERNEL_MODE
    }

#ifdef SCRIPT_ENGINE_USER_MODE
    ShowMessages("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE
    return HasError;

  case FUNC_PRINT:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    //
    // Call the target function
    //
    ScriptEngineFunctionPrint(ActionDetail.Tag,
                              ActionDetail.ImmediatelySendTheResults, SrcVal0);
    return HasError;

  case FUNC_DISABLEEVENT:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    ScriptEngineFunctionDisableEvent(
        ActionDetail.Tag, ActionDetail.ImmediatelySendTheResults, SrcVal0);
    return HasError;

  case FUNC_ENABLEEVENT:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);
    ScriptEngineFunctionEnableEvent(
        ActionDetail.Tag, ActionDetail.ImmediatelySendTheResults, SrcVal0);
    return HasError;

  case FUNC_FORMATS:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

    //
    // Call the target function
    //
    ScriptEngineFunctionFormats(
        ActionDetail.Tag, ActionDetail.ImmediatelySendTheResults, SrcVal0);
    return HasError;

  case FUNC_PRINTF:
    Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal0 =
        GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

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

    if (Src1->Value > 0) {
      Src2 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                       (unsigned long long)(*Indx * sizeof(SYMBOL)));

      *Indx = *Indx + Src1->Value;
    }
    ScriptEngineFunctionPrintf(
        GuestRegs, ActionDetail, g_TempList, g_VariableList, ActionDetail.Tag,
        ActionDetail.ImmediatelySendTheResults, (char *)&Src0->Value,
        Src1->Value, Src2, (BOOLEAN *)&HasError);

    return HasError;
  }
}
