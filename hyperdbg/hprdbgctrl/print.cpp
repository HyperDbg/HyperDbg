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

extern "C" {
__declspec(dllimport) void Parse(char *str);
}

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

//
// test function
//

VOID PerformAction(PGUEST_REGS GuestRegs, string Expr) {

  //
  // Test Parst
  //
  Parse((char *)Expr.c_str());
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
