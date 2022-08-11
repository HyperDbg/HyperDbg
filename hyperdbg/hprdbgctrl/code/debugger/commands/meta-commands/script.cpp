/**
 * @file script.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief .script command
 * @details
 * @version 0.1
 * @date 2020-07-03
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

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
    ShowMessages(".script : runs a HyperDbg script.\n\n");

    ShowMessages("syntax : \.script [FilePath (string)] [Args (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : .script C:\\scripts\\script.ds\n");
    ShowMessages("\t\te.g : .script C:\\scripts\\script.ds 95 85 @rsp\n");
    ShowMessages("\t\te.g : .script \"C:\\scripts\\hello world.ds\"\n");
    ShowMessages("\t\te.g : .script \"C:\\scripts\\hello world.ds\" @rax\n");
    ShowMessages("\t\te.g : .script \"C:\\scripts\\hello world.ds\" @rax @rcx+55 $pid\n");
    ShowMessages("\t\te.g : .script \"C:\\scripts\\hello world.ds\" 12 55 @rip\n");
}

/**
 * @brief Run the command
 *
 * @return VOID
 */
VOID
CommandScriptRunCommand(std::string Input, vector<string> PathAndArgs)
{
    int    CommandExecutionResult = 0;
    char * LineContent            = NULL;
    int    i                      = 0;

    //
    // Replace the $arg*s
    // This is not a good approach to replace between strings,
    // but we let it work this way and in the future versions
    // we'll integrate the command parsing in the debugger with
    // the script engine's command parser
    //
    for (auto item : PathAndArgs)
    {
        string ToReplace = "$arg" + std::to_string(i);
        i++;

        ReplaceAll(Input, ToReplace, item);
    }

    //
    // Convert script to char*
    //
    LineContent = (char *)Input.c_str();

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
    HyperDbgShowSignature();

    ShowMessages("%s\n", LineContent);

    CommandExecutionResult = HyperDbgInterpreter(LineContent);

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
 * @brief Read file and run the script
 *
 * @return VOID
 */
VOID
HyperDbgScriptReadFileAndExecuteCommand(std::vector<std::string> & PathAndArgs)
{
    std::string Line;
    BOOLEAN     IsOpened         = FALSE;
    bool        Reset            = false;
    string      CommandToExecute = "";
    string      PathOfScriptFile = "";

    //
    // Parse the script file,
    // the first argument is the path
    //
    PathOfScriptFile = PathAndArgs.at(0);
    ReplaceAll(PathOfScriptFile, "\"", "");

    ifstream File(PathOfScriptFile);

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
            CommandScriptRunCommand(CommandToExecute, PathAndArgs);

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
            CommandScriptRunCommand(CommandToExecute, PathAndArgs);

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
        ShowMessages("err, invalid file specified for the script\n");
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
    // Parse the file and the possible arguments
    //
    HyperDbgScriptReadFileAndExecuteCommand(PathAndArgs);
}
