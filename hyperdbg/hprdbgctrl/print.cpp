/**
 * @file print.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @author M.H. Gholamrezei (gholamrezaei.mh@gmail.com)
 * @brief print command
 * @details
 * @version 0.1
 * @date 2020-10-08
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Include parser
//
#define SCRIPT_ENGINE_USER_MODE
#include "ScriptEngineCommon.h"

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

//
// test function
//
VOID PerformAction(PGUEST_REGS_USER_MODE GuestRegs, string Expr) {

  //
  // Test Parser
  //
  PSYMBOL_BUFFER CodeBuffer = ScriptEngineParse((char *)Expr.c_str());
  if (CodeBuffer->Message == NULL) {
    PrintSymbolBuffer(CodeBuffer);

    for (int i = 0; i < CodeBuffer->Pointer;) {
      printf("%d\n", i);

      ScriptEngineExecute(GuestRegs, CodeBuffer, &i);
    }
    RemoveSymbolBuffer(CodeBuffer);
  } else {
    printf("%s\n", CodeBuffer->Message);
  }
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

  GUEST_REGS_USER_MODE GuestRegs = {0};

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
