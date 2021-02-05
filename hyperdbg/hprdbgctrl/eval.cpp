/**
 * @file eval.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief eval (?) command
 * @details
 * @version 0.1
 * @date 2021-02-05
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
 * @brief help of ? command
 *
 * @return VOID
 */
VOID CommandEvalHelp() {
  ShowMessages("? : evaluate and execute expressions in debuggee.\n\n");
  ShowMessages("syntax : \t? [expression]\n");
  ShowMessages("\t\te.g : ? print(dq(poi(@rcx)))\n");
  ShowMessages("\t\te.g : ? json(dq(poi(@rcx)))\n");
}

/**
 * @brief handler of ? command
 *
 * @param SplittedCommand
 * @return VOID
 */
VOID CommandEval(vector<string> SplittedCommand, string Expr) {

  PVOID CodeBuffer;
  UINT64 BufferAddress;
  UINT32 BufferLength;
  UINT32 Pointer;

  if (SplittedCommand.size() == 1) {
    ShowMessages("incorrect use of '?'\n\n");
    CommandEvalHelp();
    return;
  }

  //
  // Trim the command
  //
  Trim(Expr);

  //
  // Remove first command from it
  //
  Expr.erase(0, SplittedCommand.at(0).size());

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

    //
    // Remove the buffer of script engine interpreted code
    //
    ScriptEngineWrapperRemoveSymbolBuffer(CodeBuffer);

  } else {

    //
    // It's a test
    //
    ShowMessages("Test Expression : %s \n", Expr.c_str());
    ScriptEngineWrapperTestParser(Expr);
  }
}
