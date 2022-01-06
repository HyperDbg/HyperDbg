/**
 * @file script.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief .script command
 * @details
 * @version 0.1
 * @date 2020-07-03
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "..\hprdbgctrl\pch.h"

using namespace std;

//
// Global Variables
//
extern BOOLEAN g_ExecutingScript;

/**
 * @brief help of .script command
 *
 * @return VOID
 */
VOID
CommandScriptHelp()
{
    ShowMessages(".script : run a HyperDbg script.\n\n");
    ShowMessages("syntax : \.script [FilePath] [Arg 1..n]]\n");
    ShowMessages("\t\te.g : .script C:\\scripts\\script.dbg\n");
    ShowMessages("\t\te.g : .script C:\\scripts\\script.dbg 95 85 @rsp\n");
    ShowMessages("\t\te.g : .script list\n");
    ShowMessages("\t\te.g : .script \"C:\\scripts\\hello world.dbg\"\n");
    ShowMessages("\t\te.g : .script \"C:\\scripts\\hello world.dbg\" @rax\n");
    ShowMessages("\t\te.g : .script \"C:\\scripts\\hello world.dbg\" @rax @rcx+55 $pid\n");
    ShowMessages("\t\te.g : .script \"C:\\scripts\\hello world.dbg\" 12 55 @rip\n");
}

/**
 * @brief Run the command
 *
 * @return VOID
 */
VOID
CommandScriptRunCommand(char * LineContent)
{
    int CommandExecutionResult = 0;

    //
    // Check if the it's a command or not
    //
    InterpreterRemoveComments(LineContent);

    if (IsEmptyString(LineContent))
    {
        return;
    }

    //
    // Show current running command
    //
    HyperdbgShowSignature();

    ShowMessages("%s\n", LineContent);

    CommandExecutionResult = HyperdbgInterpreter(LineContent);

    ShowMessages("\n");

    //
    // if the debugger encounters an exit state then the return will be 1
    //
    if (CommandExecutionResult == 1)
    {
        //
        // Exit from the debugger
        //
        exit(0);
    }
}

/**
 * @brief .script command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandScript(vector<string> SplittedCommand, string Command)
{
    std::string    Line;
    BOOLEAN        IsOpened         = FALSE;
    bool           Reset            = false;
    string         CommandToExecute = "";
    vector<string> PathAndArgs;

    if (SplittedCommand.size() == 1)
    {
        ShowMessages("please specify a file\n");
        CommandScriptHelp();
        return;
    }

    //
    // Trim the command
    //
    Trim(Command);

    //
    // Remove .script from it
    //
    Command.erase(0, 7);

    //
    // Trim it again
    //
    Trim(Command);

    //
    // Split Path and args
    //
    SplitPathAndArgs(PathAndArgs, Command);

    /*
    for (auto item : PathAndArgs)
    {
        //
        // The first argument is the path
        //
        ShowMessages("Arg : %s\n", item.c_str());
    }
    */

    //
    // Parse the script file,
    // the first argument is the path
    //
    ifstream File(PathAndArgs.at(0));

    if (File.is_open())
    {
        IsOpened = TRUE;

        //
        // Indicate that it's a script
        //
        g_ExecutingScript = TRUE;

        //
        // Reset multiline command
        //
        Reset = true;

        while (std::getline(File, Line))
        {
            //
            // Check for multiline commands
            //
            if (HyperDbgCheckMultilineCommand(Line, Reset))
            {
                //
                // if the reset is true, we should make the saving buffer empty
                //
                if (Reset)
                {
                    CommandToExecute.clear();
                }

                //
                // The command is expected to be continued
                //
                Reset = false;

                //
                // Append to the previous command
                //
                CommandToExecute += Line + "\n";

                continue;
            }
            else
            {
                //
                // Reset for the next commands round
                //
                Reset = true;

                //
                // Append this line too
                //
                CommandToExecute += Line;
            }

            //
            // Run the command
            //
            CommandScriptRunCommand((char *)CommandToExecute.c_str());

            //
            // Clear the command
            //
            CommandToExecute.clear();
        }

        //
        // Check for some probably not ended commands
        //
        if (!CommandToExecute.empty())
        {
            CommandScriptRunCommand((char *)CommandToExecute.c_str());

            //
            // Clear the command
            //
            CommandToExecute.clear();
        }

        //
        // Indicate that script is finished
        //
        g_ExecutingScript = FALSE;

        File.close();
    }

    if (!IsOpened)
    {
        ShowMessages("err, invalid file specified for .script command\n");
    }
}
