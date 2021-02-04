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

using namespace std;

//
// Global Variables
//
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

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
  PVOID CodeBuffer;
  UINT64 BufferAddress;
  UINT32 BufferLength;
  UINT32 Pointer;

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

  if (g_IsSerialConnectedToRemoteDebuggee) {

    //
    // Send over serial
    //

    //
    // Run script engine handler
    //
    CodeBuffer = ScriptEngineParseWrapper((char *)Expr.c_str());

    if (CodeBuffer == NULL) {

      //
      // return to show that this item contains an script
      //
      return;
    }

    //
    // Print symbols (test)
    //
    // PrintSymbolBufferWrapper(CodeBuffer);

    //
    // Set the buffer and length
    //
    BufferAddress = ScriptEngineWrapperGetHead(CodeBuffer);
    BufferLength = ScriptEngineWrapperGetSize(CodeBuffer);
    Pointer = ScriptEngineWrapperGetPointer(CodeBuffer);

    //
    // Send it to the remote debuggee
    //
    KdSendScriptPacketToDebuggee(BufferAddress, BufferLength, Pointer);

  } else {

    //
    // It's a test
    //
    ShowMessages("Expression : %s \n", Expr.c_str());
    ScriptEngineWrapperTestParser(Expr);
  }
}
