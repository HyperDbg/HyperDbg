/**
 * @file print.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief print command
 * @details
 * @version 0.1
 * @date 2020-10-08
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

using namespace std;

VOID TestParser(string Expr);

/**
 * @brief help of print command
 *
 * @return VOID
 */
VOID CommandPrintHelp() {
  ShowMessages("print : evaluate expressions.\n\n");
  ShowMessages("syntax : \tprint [expression]\n");
  ShowMessages("\t\te.g : print dq(poi(@rcx))\n");
}

/**
 * @brief handler of print command
 *
 * @param SplittedCommand
 * @return VOID
 */
VOID CommandPrint(vector<string> SplittedCommand, string Expr) {

  BOOL Status;
  ULONG ReturnedLength;
  DEBUGGER_PRINT PrintRequest = {0};

  if (SplittedCommand.size() == 1) {
    ShowMessages("incorrect use of 'print'\n\n");
    CommandPrintHelp();
    return;
  }

  //
  // Trim the command
  //
  Trim(Expr);

  //
  // Remove print from it
  //
  Expr.erase(0, 5);

  //
  // Trim it again
  //
  Trim(Expr);

  //
  // TODO: end of string must have a whitspace. fix it.
  //
  Expr.append(" ");
  // Expr = " x = 4 >> 1; ";

  printf("Expression : %s \n", Expr.c_str());
  TestParser(Expr);
  return;

  if (!g_DeviceHandle) {
    ShowMessages("Handle not found, probably the driver is not loaded. Did you "
                 "use 'load' command?\n");
    return;
  }

  //
  // Send the request to the kernel
  //
  Status = DeviceIoControl(g_DeviceHandle,        // Handle to device
                           IOCTL_DEBUGGER_PRINT,  // IO Control coder
                           &PrintRequest,         // Input Buffer to driver.
                           SIZEOF_DEBUGGER_PRINT, // Input buffer length
                           &PrintRequest,         // Output Buffer from driver.
                           SIZEOF_DEBUGGER_PRINT, // Length of output buffer
                                                  // in bytes.
                           &ReturnedLength,       // Bytes placed in buffer.
                           NULL                   // synchronous call
  );

  if (!Status) {
    ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
    return;
  }
}
////////////////////////////////////////////////////////////////////
//                  Test (Should be removed)
////////////////////////////////////////////////////////////////////

typedef unsigned long long QWORD;

#define LOWORD(l) ((WORD)(l))
#define HIWORD(l) ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w) ((BYTE)(w))
#define HIBYTE(w) ((BYTE)(((WORD)(w) >> 8) & 0xFF))

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

#define MAX_TEMP_COUNT 32

// TODO: Extract number of variables from input of ScriptEngine
// and allocate variableList Dynamically.
#define MAX_VAR_COUNT 32

typedef struct _GUEST_REGS {
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
} GUEST_REGS, *PGUEST_REGS;

UINT64 TempList[MAX_TEMP_COUNT] = {0};
UINT64 VariableList[MAX_VAR_COUNT] = {0};

//
// Pseudo registers
//

// $tid
UINT64 ScriptEnginePseudoRegGetTid() { return GetCurrentThreadId(); }

// $pid
UINT64 ScriptEnginePseudoRegGetPid() { return GetCurrentProcessId(); }

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

UINT64 GetRegValue(PGUEST_REGS GuestRegs, PSYMBOL Symbol) {
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
    printf("Error in reading regesiter");
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
    printf("Error in reading regesiter");
    return INVALID;
    // TODO: Add all the register
  }
}
UINT64 GetValue(PGUEST_REGS GuestRegs, PSYMBOL Symbol) {

  switch (Symbol->Type) {
  case SYMBOL_ID_TYPE:
    return VariableList[Symbol->Value];
  case SYMBOL_NUM_TYPE:
    return Symbol->Value;
  case SYMBOL_REGISTER_TYPE:
    return GetRegValue(GuestRegs, Symbol);
  case SYMBOL_PSEUDO_REG_TYPE:
    return GetPseudoRegValue(Symbol);
  case SYMBOL_TEMP_TYPE:
    return TempList[Symbol->Value];
  }
}
VOID SetValue(PGUEST_REGS GuestRegs, PSYMBOL Symbol, UINT64 Value) {
  switch (Symbol->Type) {
  case SYMBOL_ID_TYPE:
    VariableList[Symbol->Value] = Value;
    return;
  case SYMBOL_TEMP_TYPE:
    TempList[Symbol->Value] = Value;
    return;
  }
}

//
VOID ScriptEngineExecute(PGUEST_REGS GuestRegs, PSYMBOL_BUFFER CodeBuffer,
                         int &Indx) {
  PSYMBOL Operator;
  PSYMBOL Src0;
  PSYMBOL Src1;
  PSYMBOL Des;
  UINT64 SrcVal0;
  UINT64 SrcVal1;
  UINT64 DesVal;

  Operator = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                       (unsigned long long)(Indx * sizeof(SYMBOL)));
  Indx++;
  if (Operator->Type != SYMBOL_SEMANTIC_RULE_TYPE) {
    printf("Error:Expecting Operator Type.\n");
  }

  Src0 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                   (unsigned long long)(Indx * sizeof(SYMBOL)));
  Indx++;
  SrcVal0 = GetValue(GuestRegs, Src0);
  switch (Operator->Value) {
  case FUNC_OR:
    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;

    SrcVal1 = GetValue(GuestRegs, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;

    DesVal = SrcVal1 | SrcVal0;
    SetValue(GuestRegs, Des, DesVal);
    printf("DesVal = %d\n", DesVal);

    return;

  case FUNC_XOR:
    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;

    SrcVal1 = GetValue(GuestRegs, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;

    DesVal = SrcVal1 ^ SrcVal0;
    SetValue(GuestRegs, Des, DesVal);
    printf("DesVal = %d\n", DesVal);

    return;

  case FUNC_AND:
    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;

    SrcVal1 = GetValue(GuestRegs, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;

    DesVal = SrcVal1 & SrcVal0;
    SetValue(GuestRegs, Des, DesVal);
    printf("DesVal = %d\n", DesVal);

    return;

  case FUNC_ASR:
    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;

    SrcVal1 = GetValue(GuestRegs, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;

    DesVal = SrcVal1 >> SrcVal0;
    SetValue(GuestRegs, Des, DesVal);
    printf("DesVal = %d\n", DesVal);

    return;

  case FUNC_ASL:
    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;

    SrcVal1 = GetValue(GuestRegs, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;

    DesVal = SrcVal1 << SrcVal0;
    SetValue(GuestRegs, Des, DesVal);
    printf("DesVal = %d\n", DesVal);

    return;

  case FUNC_ADD:
    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;
    SrcVal1 = GetValue(GuestRegs, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;

    DesVal = SrcVal1 + SrcVal0;
    SetValue(GuestRegs, Des, DesVal);
    printf("DesVal = %d\n", DesVal);

    return;
  case FUNC_SUB:
    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;
    SrcVal1 = GetValue(GuestRegs, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;

    DesVal = SrcVal1 - SrcVal0;
    SetValue(GuestRegs, Des, DesVal);
    printf("DesVal = %d\n", DesVal);

    return;
  case FUNC_MUL:
    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;
    SrcVal1 = GetValue(GuestRegs, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;

    DesVal = SrcVal1 * SrcVal0;
    SetValue(GuestRegs, Des, DesVal);
    printf("DesVal = %d\n", DesVal);

    return;

  case FUNC_DIV:
    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;
    SrcVal1 = GetValue(GuestRegs, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;

    DesVal = SrcVal1 / SrcVal0;
    SetValue(GuestRegs, Des, DesVal);
    printf("DesVal = %d\n", DesVal);

    return;
  case FUNC_MOD:
    Src1 = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                     (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;
    SrcVal1 = GetValue(GuestRegs, Src1);

    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;

    DesVal = SrcVal1 % SrcVal0;
    SetValue(GuestRegs, Des, DesVal);
    printf("DesVal = %d\n", DesVal);

    return;

  case FUNC_POI:
    // TODO: Hanlde poi function
    printf("Error: Poi functions is not handled yet.\n");

    return;

  case FUNC_DB:
    // TODO: Hanlde db function
    printf("Error: DB functions is not handled yet.\n");

    return;
  case FUNC_DW:
    // TODO: Hanlde dw function
    printf("Error: DW functions is not handled yet.\n");

    return;
  case FUNC_DQ:
    // TODO: Hanlde dq function
    printf("Error: Dq functions is not handled yet.\n");

    return;

  case FUNC_STR:
    // TODO: Hanlde str function
    printf("Error: STR functions is not handled yet.\n");

    return;

  case FUNC_WSTR:
    // TODO: Hanlde wstr function
    printf("Error: WSTR functions is not handled yet.\n");

    return;

  case FUNC_SIZEOF:
    // TODO: Hanlde sizeof function
    printf("Error: DB functions is not handled yet.\n");

    return;

  case FUNC_NOT:
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;

    DesVal = ~SrcVal0;
    SetValue(GuestRegs, Des, DesVal);
    printf("DesVal = %d\n", DesVal);

    return;

  case FUNC_NEG:
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;

    DesVal = -(INT64)SrcVal0;
    SetValue(GuestRegs, Des, DesVal);
    printf("DesVal = %d\n", DesVal);

    return;

  case FUNC_HI:
    // TODO: Hanlde hi function
    printf("Error: HI functions is not handled yet.\n");

    return;

  case FUNC_LOW:
    // TODO: Hanlde low function
    printf("Error: LOW functions is not handled yet.\n");

    return;

  case FUNC_MOV:
    Des = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                    (unsigned long long)(Indx * sizeof(SYMBOL)));
    Indx++;

    DesVal = SrcVal0;
    SetValue(GuestRegs, Des, DesVal);
    printf("DesVal = %d\n", DesVal);

    return;
  }
}
//
// test function
//

VOID PerformAction(PGUEST_REGS GuestRegs, string Expr) {

  //
  // Test Parser
  //
  PSYMBOL_BUFFER CodeBuffer = ScriptEngineParse((char *)Expr.c_str());

  PrintSymbolBuffer(CodeBuffer);

  for (int i = 0; i < CodeBuffer->Pointer;) {
    printf("%d\n", i);

    ScriptEngineExecute(GuestRegs, CodeBuffer, i);
  }

  RemoveSymbolBuffer(CodeBuffer);
}

VOID TestParser(string Expr) {

  typedef struct _TEST_STRUCT {
    UINT64 Var1;
    UINT64 Var2;
    UINT64 Var3;
    UINT64 Var4;
  } TEST_STRUCT, *PTEST_STRUCT;

  PTEST_STRUCT TestStruct = (PTEST_STRUCT)malloc(sizeof(TEST_STRUCT));
  RtlZeroMemory(TestStruct, sizeof(TEST_STRUCT));

  TestStruct->Var1 = 0x41414141;
  TestStruct->Var3 = 0x4242424242424242;

  GUEST_REGS GuestRegs = {0};

  GuestRegs.rax = 0x1;
  GuestRegs.rcx = (UINT64)TestStruct;
  GuestRegs.rdx = 0x3;
  GuestRegs.rbx = 0x4;
  GuestRegs.rsp = 0x5;
  GuestRegs.rbp = 0x6;
  GuestRegs.rsi = 0x7;
  GuestRegs.rdi = 0x8;
  GuestRegs.r8 = 0x9;
  GuestRegs.r9 = 0xa;
  GuestRegs.r10 = 0xb;
  GuestRegs.r11 = 0xc;
  GuestRegs.r12 = 0xd;
  GuestRegs.r13 = 0xe;
  GuestRegs.r14 = 0xf;
  GuestRegs.r15 = 0x10;

  PerformAction(&GuestRegs, Expr);
}
