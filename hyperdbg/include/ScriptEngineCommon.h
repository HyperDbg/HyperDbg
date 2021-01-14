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
//
// Wrapper headers
//
#ifdef SCRIPT_ENGINE_KERNEL_MODE

UINT64
ScriptEngineWrapperGetInstructionPointer();

UINT64
ScriptEngineWrapperGetAddressOfReservedBuffer(PDEBUGGER_EVENT_ACTION Action);

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

  /* printf("PEB : %p\n", PebPtr); */

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
UINT64 ScriptEngineKeywordPoi(PUINT64 Address) { return *Address; }

// hi
WORD ScriptEngineKeywordHi(PUINT64 Address) {
  QWORD Result = *Address;
  return HIWORD(Result);
}

// low
WORD ScriptEngineKeywordLow(PUINT64 Address) {
  QWORD Result = *Address;
  return LOWORD(Result);
}

// db
BYTE ScriptEngineKeywordDb(PUINT64 Address) {
  BYTE Result = *Address;
  return Result;
}

// dd
WORD ScriptEngineKeywordDd(PUINT64 Address) {
  WORD Result = *Address;
  return Result;
}

// dw
DWORD ScriptEngineKeywordDw(PUINT64 Address) {
  DWORD Result = *Address;
  return Result;
}

// dq
QWORD ScriptEngineKeywordDq(PUINT64 Address) {
  QWORD Result = *Address;
  return Result;
}

// str
CHAR *ScriptEngineKeywordStr(CHAR *Address) {

  UINT64 Len = 0;

  Len = strlen(Address) + 1;

  if (Len == 1) {
    return NULL;
  } else {
    return Address;
  }
}

// wstr
WCHAR *ScriptEngineKeywordWstr(WCHAR *Address) {

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
  printf("%llx\n", Value);

#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
  LogSimpleWithTag(Tag, ImmediateMessagePassing, "%llx\n", Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE
}

VOID ScriptEngineFunctionJson(UINT64 Tag, BOOLEAN ImmediateMessagePassing,
                              char *Name, UINT64 Value) {

#ifdef SCRIPT_ENGINE_USER_MODE
  printf("%s : %d\n", Name, Value);
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
  LogSimpleWithTag(Tag, ImmediateMessagePassing, "%s : %d\n", Name, Value);
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
    printf("Error in reading regesiter");
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
    return ScriptEnginePseudoRegGetBuffer((UINT64 *)ActionBuffer.CurrentAction);
  case PSEUDO_REGISTER_CONTEXT:
    return ActionBuffer.Context;
  case INVALID:
#ifdef SCRIPT_ENGINE_USER_MODE
    printf("Error in reading regesiter");
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

//
VOID ScriptEngineExecute(PGUEST_REGS_USER_MODE GuestRegs,
                         ACTION_BUFFER ActionDetail, UINT64 *g_TempList,
                         UINT64 *g_VariableList, PSYMBOL_BUFFER CodeBuffer,
                         int *Indx) {

  PSYMBOL Operator;
  PSYMBOL Src0;
  PSYMBOL Src1;
  PSYMBOL Des;
  UINT64 SrcVal0;
  UINT64 SrcVal1;
  UINT64 DesVal;

  Operator = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                       (unsigned long long)(*Indx * sizeof(SYMBOL)));
  *Indx = *Indx + 1;
  if (Operator->Type != SYMBOL_SEMANTIC_RULE_TYPE) {

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("Error:Expecting Operator Type.\n");
#endif // SCRIPT_ENGINE_USER_MODE
  }

  Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                   (unsigned long long)(*Indx * sizeof(SYMBOL)));
  *Indx = *Indx + 1;
  SrcVal0 = GetValue(GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0);

  switch (Operator->Value) {
  case FUNC_OR:
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
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_XOR:
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
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_AND:
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
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_ASR:
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
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_ASL:
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
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_ADD:
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
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_SUB:
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
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;
  case FUNC_MUL:
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
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_DIV:
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
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;
  case FUNC_MOD:
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
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_POI:
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = ScriptEngineKeywordPoi((PUINT64)GetValue(
        GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0));
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_DB:
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = ScriptEngineKeywordDb((PUINT64)GetValue(
        GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0));
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;
  case FUNC_DW:
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = ScriptEngineKeywordDb((PUINT64)GetValue(
        GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0));
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;
  case FUNC_DQ:
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = ScriptEngineKeywordDq((PUINT64)GetValue(
        GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0));
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_STR:
    // TODO: Hanlde str function

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("Error: STR functions is not handled yet.\n");
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_WSTR:
    // TODO: Hanlde wstr function

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("Error: WSTR functions is not handled yet.\n");
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_SIZEOF:
    // TODO: Hanlde sizeof function because we do not support pdb so
    // we don't have support this function
#ifdef SCRIPT_ENGINE_USER_MODE
    printf("Error: DB functions is not handled yet.\n");
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_NOT:
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = ~SrcVal0;
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_NEG:
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = -(INT64)SrcVal0;
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE
    return;

  case FUNC_HI:
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = ScriptEngineKeywordHi((PUINT64)GetValue(
        GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0));
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE
    return;

  case FUNC_LOW:
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = ScriptEngineKeywordLow((PUINT64)GetValue(
        GuestRegs, ActionDetail, g_TempList, g_VariableList, Src0));
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE
    return;
  case FUNC_MOV:
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal0;
    SetValue(GuestRegs, g_TempList, g_VariableList, Des, DesVal);
    if (Des->Type == SYMBOL_ID_TYPE) {
#ifdef SCRIPT_ENGINE_USER_MODE
      printf("Result is %llx\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
      DbgBreakPoint();
      LogInfo("Result is %llx\n", DesVal);
#endif // SCRIPT_ENGINE_KERNEL_MODE
    }

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE
    return;

  case FUNC_PRINT:

    //
    // Call the target function
    //
    ScriptEngineFunctionPrint(ActionDetail.Tag,
                              ActionDetail.ImmediatelySendTheResults, SrcVal0);
    return;

  case FUNC_JSON:

    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx =
        *Indx + ((sizeof(unsigned long long) + strlen((char *)&Src1->Value)) /
                     sizeof(SYMBOL) +
                 1);

    ScriptEngineFunctionJson(ActionDetail.Tag,
                             ActionDetail.ImmediatelySendTheResults,
                             (char *)&Src1->Value, SrcVal0);

    return;
  }
}
