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

typedef signed char INT8, *PINT8;
typedef signed short INT16, *PINT16;
typedef signed int INT32, *PINT32;
typedef signed __int64 INT64, *PINT64;
typedef unsigned char UINT8, *PUINT8;
typedef unsigned short UINT16, *PUINT16;
typedef unsigned int UINT32, *PUINT32;
typedef unsigned __int64 UINT64, *PUINT64;

typedef struct SYMBOL {
  long long unsigned Type;
  long long unsigned Value;
} SYMBOL, *PSYMBOL;

typedef struct SYMBOL_BUFFER {
  PSYMBOL Head;
  unsigned int Pointer;
  unsigned int Size;
  char *Message;

} SYMBOL_BUFFER, *PSYMBOL_BUFFER;

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

#define FUNC_OR 0
#define FUNC_XOR 1
#define FUNC_AND 2
#define FUNC_ASR 3
#define FUNC_ASL 4
#define FUNC_ADD 5
#define FUNC_SUB 6
#define FUNC_MUL 7
#define FUNC_DIV 8
#define FUNC_MOD 9
#define FUNC_POI 10
#define FUNC_DB 11
#define FUNC_DD 12
#define FUNC_DW 13
#define FUNC_DQ 14
#define FUNC_STR 15
#define FUNC_WSTR 16
#define FUNC_SIZEOF 17
#define FUNC_NOT 18
#define FUNC_NEG 19
#define FUNC_HI 20
#define FUNC_LOW 21
#define FUNC_MOV 22
#define FUNC_PRINT 23

#define SYMBOL_ID_TYPE 0
#define SYMBOL_NUM_TYPE 1
#define SYMBOL_REGISTER_TYPE 2
#define SYMBOL_PSEUDO_REG_TYPE 3
#define SYMBOL_SEMANTIC_RULE_TYPE 4
#define SYMBOL_TEMP_TYPE 5

#define RAX_MNEMONIC 0
#define RCX_MNEMONIC 1
#define RDX_MNEMONIC 2
#define RBX_MNEMONIC 3
#define RSP_MNEMONIC 4
#define RBP_MNEMONIC 5
#define RSI_MNEMONIC 6
#define RDI_MNEMONIC 7
#define R8_MNEMONIC 8
#define R9_MNEMONIC 9
#define R10_MNEMONIC 10
#define R11_MNEMONIC 11
#define R12_MNEMONIC 12
#define R13_MNEMONIC 13
#define R14_MNEMONIC 14
#define R15_MNEMONIC 15

#define INVALID -1

#define TID_MNEMONIC 0
#define PID_MNEMONIC 1

#define SYMBOL_ID_TYPE 0
#define SYMBOL_NUM_TYPE 1
#define SYMBOL_REGISTER_TYPE 2
#define SYMBOL_PSEUDO_REG_TYPE 3
#define SYMBOL_SEMANTIC_RULE_TYPE 4
#define SYMBOL_TEMP 5

#define LOWORD(l) ((WORD)(l))
#define HIWORD(l) ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w) ((BYTE)(w))
#define HIBYTE(w) ((BYTE)(((WORD)(w) >> 8) & 0xFF))

#define MAX_TEMP_COUNT 32

// TODO: Extract number of variables from input of ScriptEngine
// and allocate variableList Dynamically.
#define MAX_VAR_COUNT 32

UINT64 g_TempList[MAX_TEMP_COUNT] = {0};
UINT64 g_VariableList[MAX_VAR_COUNT] = {0};

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

UINT64 GetRegValue(PGUEST_REGS_USER_MODE GuestRegs, PSYMBOL Symbol) {
  switch (Symbol->Value) {
  case RAX_MNEMONIC:
    return GuestRegs->rax;
  case RCX_MNEMONIC:
    return GuestRegs->rcx;
  case RDX_MNEMONIC:
    return GuestRegs->rdx;
  case RBX_MNEMONIC:
    return GuestRegs->rbx;
  case RSP_MNEMONIC:
    return GuestRegs->rsp;
  case RBP_MNEMONIC:
    return GuestRegs->rbp;
  case RSI_MNEMONIC:
    return GuestRegs->rsi;
  case RDI_MNEMONIC:
    return GuestRegs->rdi;
  case R8_MNEMONIC:
    return GuestRegs->r8;
  case R9_MNEMONIC:
    return GuestRegs->r9;
  case R10_MNEMONIC:
    return GuestRegs->r10;
  case R11_MNEMONIC:
    return GuestRegs->r11;
  case R12_MNEMONIC:
    return GuestRegs->r12;
  case R13_MNEMONIC:
    return GuestRegs->r13;
  case R14_MNEMONIC:
    return GuestRegs->r14;
  case R15_MNEMONIC:
    return GuestRegs->r15;
  case INVALID:
#ifdef SCRIPT_ENGINE_USER_MODE
    printf("Error in reading regesiter");
#endif // SCRIPT_ENGINE_USER_MODE
    return INVALID;
    // TODO: Add all the register
  }
}
UINT64 GetPseudoRegValue(PSYMBOL Symbol) {
  switch (Symbol->Value) {
  case TID_MNEMONIC:
    return ScriptEnginePseudoRegGetTid();
  case PID_MNEMONIC:
    return ScriptEnginePseudoRegGetPid();
  case INVALID:
#ifdef SCRIPT_ENGINE_USER_MODE
    printf("Error in reading regesiter");
#endif // SCRIPT_ENGINE_USER_MODE
    return INVALID;
    // TODO: Add all the register
  }
}
UINT64 GetValue(PGUEST_REGS_USER_MODE GuestRegs, PSYMBOL Symbol) {

  switch (Symbol->Type) {
  case SYMBOL_ID_TYPE:
    return g_VariableList[Symbol->Value];
  case SYMBOL_NUM_TYPE:
    return Symbol->Value;
  case SYMBOL_REGISTER_TYPE:
    return GetRegValue(GuestRegs, Symbol);
  case SYMBOL_PSEUDO_REG_TYPE:
    return GetPseudoRegValue(Symbol);
  case SYMBOL_TEMP_TYPE:
    return g_TempList[Symbol->Value];
  }
}

VOID SetValue(PGUEST_REGS_USER_MODE GuestRegs, PSYMBOL Symbol, UINT64 Value) {
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
                         PSYMBOL_BUFFER CodeBuffer, int *Indx) {

  PSYMBOL Operator;
  PSYMBOL Src0;
  PSYMBOL Src1;
  PSYMBOL Des;
  UINT64 SrcVal0;
  UINT64 SrcVal1;
  UINT64 DesVal;

#ifdef SCRIPT_ENGINE_KERNEL_MODE
  DbgBreakPoint();
#endif

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
  SrcVal0 = GetValue(GuestRegs, Src0);

  switch (Operator->Value) {
  case FUNC_OR:
    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    SrcVal1 = GetValue(GuestRegs, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal1 | SrcVal0;
    SetValue(GuestRegs, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_XOR:
    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    SrcVal1 = GetValue(GuestRegs, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal1 ^ SrcVal0;
    SetValue(GuestRegs, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_AND:
    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    SrcVal1 = GetValue(GuestRegs, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal1 & SrcVal0;
    SetValue(GuestRegs, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_ASR:
    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    SrcVal1 = GetValue(GuestRegs, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal1 >> SrcVal0;
    SetValue(GuestRegs, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_ASL:
    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    SrcVal1 = GetValue(GuestRegs, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal1 << SrcVal0;
    SetValue(GuestRegs, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_ADD:
    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal1 = GetValue(GuestRegs, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal1 + SrcVal0;
    SetValue(GuestRegs, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_SUB:
    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal1 = GetValue(GuestRegs, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal1 - SrcVal0;
    SetValue(GuestRegs, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;
  case FUNC_MUL:
    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal1 = GetValue(GuestRegs, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal1 * SrcVal0;
    SetValue(GuestRegs, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_DIV:
    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal1 = GetValue(GuestRegs, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal1 / SrcVal0;
    SetValue(GuestRegs, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;
  case FUNC_MOD:
    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;
    SrcVal1 = GetValue(GuestRegs, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal1 % SrcVal0;
    SetValue(GuestRegs, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_POI:
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = ScriptEngineKeywordPoi((PUINT64)GetValue(GuestRegs, Src0));
    SetValue(GuestRegs, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_DB:
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = ScriptEngineKeywordDb((PUINT64)GetValue(GuestRegs, Src0));
    SetValue(GuestRegs, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;
  case FUNC_DW:
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = ScriptEngineKeywordDb((PUINT64)GetValue(GuestRegs, Src0));
    SetValue(GuestRegs, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;
  case FUNC_DQ:
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = ScriptEngineKeywordDq((PUINT64)GetValue(GuestRegs, Src0));
    SetValue(GuestRegs, Des, DesVal);

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
    SetValue(GuestRegs, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE

    return;

  case FUNC_NEG:
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = -(INT64)SrcVal0;
    SetValue(GuestRegs, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE
    return;

  case FUNC_HI:
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = ScriptEngineKeywordHi((PUINT64)GetValue(GuestRegs, Src0));
    SetValue(GuestRegs, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE
    return;

  case FUNC_LOW:
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = ScriptEngineKeywordLow((PUINT64)GetValue(GuestRegs, Src0));
    SetValue(GuestRegs, Des, DesVal);

#ifdef SCRIPT_ENGINE_USER_MODE
    printf("DesVal = %d\n", DesVal);
#endif // SCRIPT_ENGINE_USER_MODE
    return;
  case FUNC_MOV:
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal0;
    SetValue(GuestRegs, Des, DesVal);
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
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(*Indx * sizeof(SYMBOL)));
    *Indx = *Indx + 1;

    DesVal = SrcVal0;
    SetValue(GuestRegs, Des, DesVal);
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
  }
}
