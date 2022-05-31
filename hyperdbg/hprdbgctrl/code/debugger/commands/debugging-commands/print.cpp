/**
 * @file print.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
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
VOID
CommandPrintHelp()
{
    ShowMessages("print : evaluates expressions.\n\n");

    ShowMessages("syntax : \tprint [Expression (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : print dq(poi(@rcx))\n");
}

/**
 * @brief handler of print command
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandPrint(vector<string> SplittedCommand, string Command)
{
    PVOID  CodeBuffer;
    UINT64 BufferAddress;
    UINT32 BufferLength;
    UINT32 Pointer;

    if (SplittedCommand.size() == 1)
    {
        ShowMessages("incorrect use of 'print'\n\n");
        CommandPrintHelp();
        return;
    }

    //
    // Trim the command
    //
    Trim(Command);

    //
    // Remove print from it
    //
    Command.erase(0, 5);

    //
    // Trim it again
    //
    Trim(Command);

    //
    // Prepend and append 'print(' and ')'
    //
    Command.insert(0, "print(");
    Command.append(");");

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Send over serial
        //

        //
        // Run script engine handler
        //
        CodeBuffer = ScriptEngineParseWrapper((char *)Command.c_str(), TRUE);

        if (CodeBuffer == NULL)
        {
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
        BufferLength  = ScriptEngineWrapperGetSize(CodeBuffer);
        Pointer       = ScriptEngineWrapperGetPointer(CodeBuffer);

        //
        // Send it to the remote debuggee
        //
        KdSendScriptPacketToDebuggee(BufferAddress, BufferLength, Pointer, FALSE);

        //
        // Remove the buffer of script engine interpreted code
        //
        ScriptEngineWrapperRemoveSymbolBuffer(CodeBuffer);

        ShowMessages("\n");
    }
    else
    {
        //
        // error
        //
        ShowMessages("err, you're not connected to any debuggee\n");
    }
}
