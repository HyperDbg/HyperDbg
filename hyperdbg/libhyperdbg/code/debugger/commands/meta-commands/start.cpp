/**
 * @file start.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief .start command
 * @details
 * @version 0.1
 * @date 2022-01-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern std::wstring g_StartCommandPath;
extern std::wstring g_StartCommandPathAndArguments;
extern BOOLEAN      g_IsSerialConnectedToRemoteDebugger;

/**
 * @brief help of the .start command
 *
 * @return VOID
 */
VOID
CommandStartHelp()
{
    ShowMessages(".start : runs a user-mode process.\n\n");

    ShowMessages("syntax : \t.start [path Path (string)] [Parameters (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : .start path c:\\reverse_eng\\my_file.exe\n");
    ShowMessages("\t\te.g : .start path \"c:\\reverse eng\\my_file.exe\"\n");
    ShowMessages("\t\te.g : .start path \"c:\\reverse eng\\my_file.exe\" \"arg1\" 2 \"arg 3\"\n");
}

/**
 * @brief .start command handler
 *
 * @param CommandTokens
 * @param Command
 * @return VOID
 */
VOID
CommandStart(vector<CommandToken> CommandTokens, string Command)
{
    vector<string> PathAndArgs;
    string         Arguments      = "";
    BOOLEAN        IsFirstCommand = FALSE;
    BOOLEAN        PathIgnored    = FALSE;
    BOOLEAN        IsNextPath     = FALSE;

    //
    // Disable user-mode debugger in this version
    //
#if ActivateUserModeDebugger == FALSE

    if (!g_IsSerialConnectedToRemoteDebugger)
    {
        ShowMessages("the user-mode debugger in VMI Mode is still in the beta version and not stable. "
                     "we decided to exclude it from this release and release it in future versions. "
                     "if you want to test the user-mode debugger in VMI Mode, you should build "
                     "HyperDbg with special instructions. But starting processes is fully supported "
                     "in the Debugger Mode.\n"
                     "(it's not recommended to use it in VMI Mode yet!)\n");
        return;
    }

#endif // !ActivateUserModeDebugger

    if (CommandTokens.size() <= 2)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandStartHelp();
        return;
    }

    //
    // Check if the first token is 'path'
    //
    if (!CompareLowerCaseStrings(CommandTokens.at(1), "path"))
    {
        ShowMessages("err, couldn't resolve error at '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(1)).c_str());
        CommandStartHelp();
        return;
    }

    //
    // *** It's a run of target PE file ***
    //

    for (auto Section : CommandTokens)
    {
        //
        // Skip first two parameters
        //
        if (!IsFirstCommand)
        {
            IsFirstCommand = TRUE;
            continue;
        }

        //
        // Skip the path
        //
        if (!PathIgnored)
        {
            PathIgnored = TRUE;
            IsNextPath  = TRUE;
            continue;
        }

        //
        // Check if the next token is the actual path
        //
        if (IsNextPath)
        {
            IsNextPath = FALSE;

            //
            // Convert path to wstring
            //
            StringToWString(g_StartCommandPath, GetCaseSensitiveStringFromCommandToken(Section));

            continue;
        }

        //
        // Append the arguments and check if arguments contain spaces then add quotes
        //
        if (GetCaseSensitiveStringFromCommandToken(Section).find(' ') != std::string::npos)
        {
            Arguments += "\"" + GetCaseSensitiveStringFromCommandToken(Section) + "\" ";
        }
        else
        {
            Arguments += GetCaseSensitiveStringFromCommandToken(Section) + " ";
        }
    }

    //
    // Perform run of the target file
    //
    if (Arguments.empty())
    {
        UdAttachToProcess(NULL,
                          g_StartCommandPath.c_str(),
                          NULL,
                          FALSE);
    }
    else
    {
        //
        // Remove the last space
        //
        Arguments.pop_back();

        //
        // Convert arguments to wstring if it's not empty
        //
        StringToWString(g_StartCommandPathAndArguments, Arguments);

        UdAttachToProcess(NULL,
                          g_StartCommandPath.c_str(),
                          (WCHAR *)g_StartCommandPathAndArguments.c_str(),
                          FALSE);
    }
}
