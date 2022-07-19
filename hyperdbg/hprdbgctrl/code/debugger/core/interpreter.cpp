/**
 * @file interpreter.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief The hyperdbg command interpreter and driver connector
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

using namespace std;

//
// Global Variables
//
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;
extern CommandType              g_CommandsList;

extern BOOLEAN g_ShouldPreviousCommandBeContinued;
extern BOOLEAN g_IsCommandListInitialized;
extern BOOLEAN g_LogOpened;
extern BOOLEAN g_ExecutingScript;
extern BOOLEAN g_IsConnectedToHyperDbgLocally;
extern BOOLEAN g_IsConnectedToRemoteDebuggee;
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN g_IsDebuggeeRunning;
extern BOOLEAN g_BreakPrintingOutput;
extern BOOLEAN g_IsInterpreterOnString;
extern BOOLEAN g_IsInterpreterPreviousCharacterABackSlash;
extern BOOLEAN g_RtmSupport;

extern UINT32 g_VirtualAddressWidth;
extern UINT32 g_InterpreterCountOfOpenCurlyBrackets;
extern ULONG  g_CurrentRemoteCore;

extern string g_ServerPort;
extern string g_ServerIp;

/**
 * @brief Interpret commands
 *
 * @param Command The text of command
 * @return int returns return zero if it was successful or non-zero if there was
 * error
 */
int
HyperDbgInterpreter(char * Command)
{
    BOOLEAN               HelpCommand       = FALSE;
    UINT64                CommandAttributes = NULL;
    CommandType::iterator Iterator;

    //
    // Check if it's the first command and whether the mapping of command is
    // initialized or not
    //
    if (!g_IsCommandListInitialized)
    {
        //
        // Initialize the debugger
        //
        InitializeDebugger();

        g_IsCommandListInitialized = TRUE;
    }

    //
    // Save the command into log open file
    //
    if (g_LogOpened && !g_ExecutingScript)
    {
        LogopenSaveToFile(Command);
        LogopenSaveToFile("\n");
    }

    //
    // Remove the comments
    //
    InterpreterRemoveComments(Command);

    //
    // Convert to string
    //
    string CommandString(Command);

    //
    // Convert to lower case
    //
    transform(CommandString.begin(), CommandString.end(), CommandString.begin(), [](unsigned char c) { return std::tolower(c); });

    vector<string> SplittedCommand {Split(CommandString, ' ')};

    //
    // Check if user entered an empty imput
    //
    if (SplittedCommand.empty())
    {
        ShowMessages("\n");
        return 0;
    }

    string FirstCommand = SplittedCommand.front();

    //
    // Read the command's attributes
    //
    CommandAttributes = GetCommandAttributes(FirstCommand);

    //
    // Check if the command needs to be continued by pressing enter
    //
    if (CommandAttributes & DEBUGGER_COMMAND_ATTRIBUTE_REPEAT_ON_ENTER)
    {
        g_ShouldPreviousCommandBeContinued = TRUE;
    }
    else
    {
        g_ShouldPreviousCommandBeContinued = FALSE;
    }

    //
    // Check and send remote command and also we check whether this
    // is a command that should be handled in this command or we can
    // send it to the remote computer, it is because in a remote connection
    // still some of the commands should be handled in the local HyperDbg
    //
    if (g_IsConnectedToRemoteDebuggee &&
        !(CommandAttributes & DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_REMOTE_CONNECTION))
    {
        //
        // Check it here because generally, we use this variable in host
        // for showing the correct signature but we won't try to block
        // other commands, the only thing is events which is blocked
        // by the remote computer itself
        //
        if (g_BreakPrintingOutput)
        {
            g_BreakPrintingOutput = FALSE;
        }

        //
        // It's a connection over network (VMI-Mode)
        //
        RemoteConnectionSendCommand(Command, strlen(Command) + 1);

        ShowMessages("\n");

        //
        // Indicate that we sent the command to the target system
        //
        return 2;
    }
    else if (g_IsSerialConnectedToRemoteDebuggee &&
             !(CommandAttributes &
               DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE))
    {
        //
        // It's a connection over serial (Debugger-Mode)
        //

        if (CommandAttributes & DEBUGGER_COMMAND_ATTRIBUTE_WONT_STOP_DEBUGGER_AGAIN)
        {
            KdSendUserInputPacketToDebuggee(Command, strlen(Command) + 1, TRUE);

            //
            // Set the debuggee to show that it's running
            //
            KdSetStatusAndWaitForPause();
        }
        else
        {
            KdSendUserInputPacketToDebuggee(Command, strlen(Command) + 1, FALSE);
        }

        //
        // Indicate that we sent the command to the target system
        //
        return 2;
    }

    //
    // Detect whether it's a .help command or not
    //
    if (!FirstCommand.compare(".help") || !FirstCommand.compare("help") ||
        !FirstCommand.compare(".hh"))
    {
        if (SplittedCommand.size() == 2)
        {
            //
            // Show that it's a help command
            //
            HelpCommand  = TRUE;
            FirstCommand = SplittedCommand.at(1);
        }
        else
        {
            ShowMessages("incorrect use of '%s'\n", FirstCommand.c_str());
            CommandHelpHelp();
            return 0;
        }
    }

    //
    // Start parsing commands
    //
    Iterator = g_CommandsList.find(FirstCommand);

    if (Iterator == g_CommandsList.end())
    {
        //
        //  Command doesn't exist
        //
        string         CaseSensitiveCommandString(Command);
        vector<string> CaseSensitiveSplittedCommand {Split(CaseSensitiveCommandString, ' ')};

        if (!HelpCommand)
        {
            ShowMessages("err, couldn't resolve command at '%s'\n", CaseSensitiveSplittedCommand.front().c_str());
        }
        else
        {
            ShowMessages("err, couldn't find the help for the command at '%s'\n",
                         CaseSensitiveSplittedCommand.at(1).c_str());
        }
    }
    else
    {
        if (HelpCommand)
        {
            Iterator->second.CommandHelpFunction();
        }
        else
        {
            //
            // Check if command is case-sensitive or not
            //
            if ((Iterator->second.CommandAttrib &
                 DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_CASE_SENSITIVE))
            {
                string CaseSensitiveCommandString(Command);
                Iterator->second.CommandFunction(SplittedCommand, CaseSensitiveCommandString);
            }
            else
            {
                Iterator->second.CommandFunction(SplittedCommand, CommandString);
            }
        }
    }

    //
    // Save the command into log open file
    //
    if (g_LogOpened && !g_ExecutingScript)
    {
        LogopenSaveToFile("\n");
    }

    return 0;
}

/**
 * @brief Remove batch comments
 *
 * @return VOID
 */
VOID
InterpreterRemoveComments(char * CommandText)
{
    BOOLEAN IsComment       = FALSE;
    BOOLEAN IsOnString      = FALSE;
    UINT32  LengthOfCommand = strlen(CommandText);

    for (size_t i = 0; i < LengthOfCommand; i++)
    {
        if (IsComment)
        {
            if (CommandText[i] == '\n')
            {
                IsComment = FALSE;
            }
            else
            {
                if (CommandText[i] != '\0')
                {
                    memmove((void *)&CommandText[i], (const void *)&CommandText[i + 1], strlen(CommandText) - i);
                    i--;
                }
            }
        }
        else if (CommandText[i] == '#' && !IsOnString)
        {
            //
            // Comment detected
            //
            IsComment = TRUE;
            i--;
        }
        else if (CommandText[i] == '"')
        {
            if (i != 0 && CommandText[i - 1] == '\\')
            {
                //
                // It's an escape character for : \"
                //
            }
            else if (IsOnString)
            {
                IsOnString = FALSE;
            }
            else
            {
                IsOnString = TRUE;
            }
        }
    }
}

/**
 * @brief Show signature of HyperDbg
 *
 * @return VOID
 */
VOID
HyperDbgShowSignature()
{
    if (g_IsConnectedToRemoteDebuggee)
    {
        //
        // Remote debugging over tcp (vmi-mode)
        //
        ShowMessages("[%s:%s] HyperDbg> ", g_ServerIp.c_str(), g_ServerPort.c_str());
    }
    else if (g_ActiveProcessDebuggingState.IsActive)
    {
        //
        // Debugging a special process
        //
        ShowMessages("%x:%x u%sHyperDbg> ",
                     g_ActiveProcessDebuggingState.ProcessId,
                     g_ActiveProcessDebuggingState.ThreadId,
                     g_ActiveProcessDebuggingState.Is32Bit ? "86" : "64");
    }
    else if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Remote debugging over serial (debugger-mode)
        //
        ShowMessages("%x: kHyperDbg> ", g_CurrentRemoteCore);
    }
    else
    {
        //
        // Anything other than above scenarios including local debugging
        // in vmi-mode
        //
        ShowMessages("HyperDbg> ");
    }
}

/**
 * @brief check for multi-line commands
 *
 * @param CurrentCommand
 * @param Reset
 * @return bool return true if the command needs extra input, otherwise
 * return false
 */
bool
HyperDbgCheckMultilineCommand(std::string & CurrentCommand, bool Reset)
{
    if (Reset)
    {
        g_IsInterpreterOnString                    = FALSE;
        g_IsInterpreterPreviousCharacterABackSlash = FALSE;
        g_InterpreterCountOfOpenCurlyBrackets      = 0;
    }

    for (size_t i = 0; i < CurrentCommand.length(); i++)
    {
        switch (CurrentCommand.at(i))
        {
        case '"':

            if (g_IsInterpreterPreviousCharacterABackSlash)
            {
                g_IsInterpreterPreviousCharacterABackSlash = FALSE;
                break; // it's an escaped \" double-quote
            }

            if (g_IsInterpreterOnString)
                g_IsInterpreterOnString = FALSE;
            else
                g_IsInterpreterOnString = TRUE;

            break;

        case '{':

            if (g_IsInterpreterPreviousCharacterABackSlash)
                g_IsInterpreterPreviousCharacterABackSlash = FALSE;

            if (!g_IsInterpreterOnString)
                g_InterpreterCountOfOpenCurlyBrackets++;

            break;

        case '}':

            if (g_IsInterpreterPreviousCharacterABackSlash)
                g_IsInterpreterPreviousCharacterABackSlash = FALSE;

            if (!g_IsInterpreterOnString && g_InterpreterCountOfOpenCurlyBrackets > 0)
                g_InterpreterCountOfOpenCurlyBrackets--;

            break;

        case '\\':

            if (g_IsInterpreterPreviousCharacterABackSlash)
                g_IsInterpreterPreviousCharacterABackSlash = FALSE; // it's not a escape character (two backslashes \\ )
            else
                g_IsInterpreterPreviousCharacterABackSlash = TRUE;

            break;

        default:

            if (g_IsInterpreterPreviousCharacterABackSlash)
                g_IsInterpreterPreviousCharacterABackSlash = FALSE;

            break;
        }
    }

    if (g_IsInterpreterOnString == FALSE && g_InterpreterCountOfOpenCurlyBrackets == 0)
    {
        //
        // either the command is finished or it's a single
        // line command
        //
        return false;
    }
    else
    {
        //
        // There still other lines, this command is incomplete
        //
        return true;
    }
}

/**
 * @brief Some of commands like stepping commands (i, p, t) and etc.
 * need to be repeated when the user press enter, this function shows
 * whether we should continue the previous command or not
 *
 * @return true means the command should be continued, false means command
 * should be ignored
 */
bool
HyperDbgContinuePreviousCommand()
{
    BOOLEAN Result = g_ShouldPreviousCommandBeContinued;

    //
    // We should keep it false for the next command
    //
    g_ShouldPreviousCommandBeContinued = FALSE;

    if (Result)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief Get Command Attributes
 *
 * @param FirstCommand just the first word of command (without other parameters)
 * @return BOOLEAN Mask of the command's attributes
 */
UINT64
GetCommandAttributes(const string & FirstCommand)
{
    CommandType::iterator Iterator;

    //
    // Some commands should not be passed to the remote system
    // and instead should be handled in the current debugger
    //

    Iterator = g_CommandsList.find(FirstCommand);

    if (Iterator == g_CommandsList.end())
    {
        //
        // Command doesn't exist, if it's not exists then it's better to handle
        // it locally, instead of sending it to the remote computer
        //
        return DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL;
    }
    else
    {
        return Iterator->second.CommandAttrib;
    }

    return NULL;
}

/**
 * @brief Initialize the debugger and adjust commands for the first run
 *
 * @return VOID
 */
VOID
InitializeDebugger()
{
    //
    // Initialize the mapping of functions
    //
    InitializeCommandsDictionary();

    //
    // Set the callback for symbol message handler
    //
    ScriptEngineSetTextMessageCallbackWrapper(ShowMessages);

    //
    // Register the CTRL+C and CTRL+BREAK Signals handler
    //
    if (!SetConsoleCtrlHandler(BreakController, TRUE))
    {
        ShowMessages("err, when registering CTRL+C and CTRL+BREAK Signals "
                     "handler\n");
        //
        // prefer to continue
        //
    }

    //
    // *** Check for feature indicators ***
    //

    //
    // Get x86 processor width for virtual address
    //
    g_VirtualAddressWidth = Getx86VirtualAddressWidth();

    //
    // Check if processor supports TSX (RTM)
    //
    g_RtmSupport = CheckCpuSupportRtm();

    //
    // Load default settings
    //
    CommandSettingsLoadDefaultValuesFromConfigFile();
}

/**
 * @brief Initialize commands and attributes
 *
 * @return VOID
 */
VOID
InitializeCommandsDictionary()
{
    g_CommandsList[".help"] = {NULL, &CommandHelpHelp, DEBUGGER_COMMAND_HELP_ATTRIBUTES};
    g_CommandsList[".hh"]   = {NULL, &CommandHelpHelp, DEBUGGER_COMMAND_HELP_ATTRIBUTES};
    g_CommandsList["help"]  = {NULL, &CommandHelpHelp, DEBUGGER_COMMAND_HELP_ATTRIBUTES};

    g_CommandsList["clear"] = {&CommandClearScreen, &CommandClearScreenHelp, DEBUGGER_COMMAND_CLEAR_ATTRIBUTES};
    g_CommandsList[".cls"]  = {&CommandClearScreen, &CommandClearScreenHelp, DEBUGGER_COMMAND_CLEAR_ATTRIBUTES};
    g_CommandsList["cls"]   = {&CommandClearScreen, &CommandClearScreenHelp, DEBUGGER_COMMAND_CLEAR_ATTRIBUTES};

    g_CommandsList[".connect"] = {&CommandConnect, &CommandConnectHelp, DEBUGGER_COMMAND_CONNECT_ATTRIBUTES};
    g_CommandsList["connect"]  = {&CommandConnect, &CommandConnectHelp, DEBUGGER_COMMAND_CONNECT_ATTRIBUTES};

    g_CommandsList[".listen"] = {&CommandListen, &CommandListenHelp, DEBUGGER_COMMAND_LISTEN_ATTRIBUTES};
    g_CommandsList["listen"]  = {&CommandListen, &CommandListenHelp, DEBUGGER_COMMAND_LISTEN_ATTRIBUTES};

    g_CommandsList["g"]  = {&CommandG, &CommandGHelp, DEBUGGER_COMMAND_G_ATTRIBUTES};
    g_CommandsList["go"] = {&CommandG, &CommandGHelp, DEBUGGER_COMMAND_G_ATTRIBUTES};

    g_CommandsList[".attach"] = {&CommandAttach, &CommandAttachHelp, DEBUGGER_COMMAND_ATTACH_ATTRIBUTES};
    g_CommandsList["attach"]  = {&CommandAttach, &CommandAttachHelp, DEBUGGER_COMMAND_ATTACH_ATTRIBUTES};

    g_CommandsList[".detach"] = {&CommandDetach, &CommandDetachHelp, DEBUGGER_COMMAND_DETACH_ATTRIBUTES};
    g_CommandsList["detach"]  = {&CommandDetach, &CommandDetachHelp, DEBUGGER_COMMAND_DETACH_ATTRIBUTES};

    g_CommandsList[".start"] = {&CommandStart, &CommandStartHelp, DEBUGGER_COMMAND_START_ATTRIBUTES};
    g_CommandsList["start"]  = {&CommandStart, &CommandStartHelp, DEBUGGER_COMMAND_START_ATTRIBUTES};

    g_CommandsList[".restart"] = {&CommandRestart, &CommandRestartHelp, DEBUGGER_COMMAND_RESTART_ATTRIBUTES};
    g_CommandsList["restart"]  = {&CommandRestart, &CommandRestartHelp, DEBUGGER_COMMAND_RESTART_ATTRIBUTES};

    g_CommandsList[".switch"] = {&CommandSwitch, &CommandSwitchHelp, DEBUGGER_COMMAND_SWITCH_ATTRIBUTES};
    g_CommandsList["switch"]  = {&CommandSwitch, &CommandSwitchHelp, DEBUGGER_COMMAND_SWITCH_ATTRIBUTES};

    g_CommandsList[".kill"] = {&CommandKill, &CommandKillHelp, DEBUGGER_COMMAND_KILL_ATTRIBUTES};
    g_CommandsList["kill"]  = {&CommandKill, &CommandKillHelp, DEBUGGER_COMMAND_KILL_ATTRIBUTES};

    g_CommandsList[".process"]  = {&CommandProcess, &CommandProcessHelp, DEBUGGER_COMMAND_PROCESS_ATTRIBUTES};
    g_CommandsList[".process2"] = {&CommandProcess, &CommandProcessHelp, DEBUGGER_COMMAND_PROCESS_ATTRIBUTES};

    g_CommandsList[".thread"]  = {&CommandThread, &CommandThreadHelp, DEBUGGER_COMMAND_THREAD_ATTRIBUTES};
    g_CommandsList[".thread2"] = {&CommandThread, &CommandThreadHelp, DEBUGGER_COMMAND_THREAD_ATTRIBUTES};

    g_CommandsList["sleep"] = {&CommandSleep, &CommandSleepHelp, DEBUGGER_COMMAND_SLEEP_ATTRIBUTES};

    g_CommandsList["event"]  = {&CommandEvents, &CommandEventsHelp, DEBUGGER_COMMAND_EVENTS_ATTRIBUTES};
    g_CommandsList["events"] = {&CommandEvents, &CommandEventsHelp, DEBUGGER_COMMAND_EVENTS_ATTRIBUTES};

    g_CommandsList["setting"]   = {&CommandSettings, &CommandSettingsHelp, DEBUGGER_COMMAND_SETTINGS_ATTRIBUTES};
    g_CommandsList["settings"]  = {&CommandSettings, &CommandSettingsHelp, DEBUGGER_COMMAND_SETTINGS_ATTRIBUTES};
    g_CommandsList[".setting"]  = {&CommandSettings, &CommandSettingsHelp, DEBUGGER_COMMAND_SETTINGS_ATTRIBUTES};
    g_CommandsList[".settings"] = {&CommandSettings, &CommandSettingsHelp, DEBUGGER_COMMAND_SETTINGS_ATTRIBUTES};

    g_CommandsList[".disconnect"] = {&CommandDisconnect, &CommandDisconnectHelp, DEBUGGER_COMMAND_DISCONNECT_ATTRIBUTES};
    g_CommandsList["disconnect"]  = {&CommandDisconnect, &CommandDisconnectHelp, DEBUGGER_COMMAND_DISCONNECT_ATTRIBUTES};

    g_CommandsList[".debug"] = {&CommandDebug, &CommandDebugHelp, DEBUGGER_COMMAND_DEBUG_ATTRIBUTES};
    g_CommandsList["debug"]  = {&CommandDebug, &CommandDebugHelp, DEBUGGER_COMMAND_DEBUG_ATTRIBUTES};

    g_CommandsList[".status"] = {&CommandStatus, &CommandStatusHelp, DEBUGGER_COMMAND_DOT_STATUS_ATTRIBUTES};
    g_CommandsList["status"]  = {&CommandStatus, &CommandStatusHelp, DEBUGGER_COMMAND_STATUS_ATTRIBUTES};

    g_CommandsList["load"] = {&CommandLoad, &CommandLoadHelp, DEBUGGER_COMMAND_LOAD_ATTRIBUTES};

    g_CommandsList["exit"]  = {&CommandExit, &CommandExitHelp, DEBUGGER_COMMAND_EXIT_ATTRIBUTES};
    g_CommandsList[".exit"] = {&CommandExit, &CommandExitHelp, DEBUGGER_COMMAND_EXIT_ATTRIBUTES};

    g_CommandsList["flush"] = {&CommandFlush, &CommandFlushHelp, DEBUGGER_COMMAND_FLUSH_ATTRIBUTES};

    g_CommandsList["pause"] = {&CommandPause, &CommandPauseHelp, DEBUGGER_COMMAND_PAUSE_ATTRIBUTES};

    g_CommandsList["unload"] = {&CommandUnload, &CommandUnloadHelp, DEBUGGER_COMMAND_UNLOAD_ATTRIBUTES};

    g_CommandsList[".script"] = {&CommandScript, &CommandScriptHelp, DEBUGGER_COMMAND_SCRIPT_ATTRIBUTES};
    g_CommandsList["script"]  = {&CommandScript, &CommandScriptHelp, DEBUGGER_COMMAND_SCRIPT_ATTRIBUTES};

    g_CommandsList["output"] = {&CommandOutput, &CommandOutputHelp, DEBUGGER_COMMAND_OUTPUT_ATTRIBUTES};

    g_CommandsList["print"] = {&CommandPrint, &CommandPrintHelp, DEBUGGER_COMMAND_PRINT_ATTRIBUTES};

    g_CommandsList["?"]        = {&CommandEval, &CommandEvalHelp, DEBUGGER_COMMAND_EVAL_ATTRIBUTES};
    g_CommandsList["eval"]     = {&CommandEval, &CommandEvalHelp, DEBUGGER_COMMAND_EVAL_ATTRIBUTES};
    g_CommandsList["evaluate"] = {&CommandEval, &CommandEvalHelp, DEBUGGER_COMMAND_EVAL_ATTRIBUTES};

    g_CommandsList[".logopen"] = {&CommandLogopen, &CommandLogopenHelp, DEBUGGER_COMMAND_LOGOPEN_ATTRIBUTES};

    g_CommandsList[".logclose"] = {&CommandLogclose, &CommandLogcloseHelp, DEBUGGER_COMMAND_LOGCLOSE_ATTRIBUTES};

    g_CommandsList["test"] = {&CommandTest, &CommandTestHelp, DEBUGGER_COMMAND_TEST_ATTRIBUTES};

    g_CommandsList["cpu"] = {&CommandCpu, &CommandCpuHelp, DEBUGGER_COMMAND_CPU_ATTRIBUTES};

    g_CommandsList["wrmsr"] = {&CommandWrmsr, &CommandWrmsrHelp, DEBUGGER_COMMAND_WRMSR_ATTRIBUTES};

    g_CommandsList["rdmsr"] = {&CommandRdmsr, &CommandRdmsrHelp, DEBUGGER_COMMAND_RDMSR_ATTRIBUTES};

    g_CommandsList["!va2pa"] = {&CommandVa2pa, &CommandVa2paHelp, DEBUGGER_COMMAND_VA2PA_ATTRIBUTES};

    g_CommandsList["!pa2va"] = {&CommandPa2va, &CommandPa2vaHelp, DEBUGGER_COMMAND_PA2VA_ATTRIBUTES};

    g_CommandsList[".formats"] = {&CommandFormats, &CommandFormatsHelp, DEBUGGER_COMMAND_FORMATS_ATTRIBUTES};
    g_CommandsList[".format"]  = {&CommandFormats, &CommandFormatsHelp, DEBUGGER_COMMAND_FORMATS_ATTRIBUTES};

    g_CommandsList["!pte"] = {&CommandPte, &CommandPteHelp, DEBUGGER_COMMAND_PTE_ATTRIBUTES};

    g_CommandsList["~"]    = {&CommandCore, &CommandCoreHelp, DEBUGGER_COMMAND_CORE_ATTRIBUTES};
    g_CommandsList["core"] = {&CommandCore, &CommandCoreHelp, DEBUGGER_COMMAND_CORE_ATTRIBUTES};

    g_CommandsList["!monitor"] = {&CommandMonitor, &CommandMonitorHelp, DEBUGGER_COMMAND_MONITOR_ATTRIBUTES};

    g_CommandsList["!vmcall"] = {&CommandVmcall, &CommandVmcallHelp, DEBUGGER_COMMAND_VMCALL_ATTRIBUTES};

    g_CommandsList["!epthook"] = {&CommandEptHook, &CommandEptHookHelp, DEBUGGER_COMMAND_EPTHOOK_ATTRIBUTES};
    g_CommandsList["bh"]       = {&CommandEptHook, &CommandEptHookHelp, DEBUGGER_COMMAND_EPTHOOK_ATTRIBUTES};

    g_CommandsList["bp"] = {&CommandBp, &CommandBpHelp, DEBUGGER_COMMAND_BP_ATTRIBUTES};

    g_CommandsList["bl"] = {&CommandBl, &CommandBlHelp, DEBUGGER_COMMAND_BD_ATTRIBUTES};

    g_CommandsList["be"] = {&CommandBe, &CommandBeHelp, DEBUGGER_COMMAND_BD_ATTRIBUTES};

    g_CommandsList["bd"] = {&CommandBd, &CommandBdHelp, DEBUGGER_COMMAND_BD_ATTRIBUTES};

    g_CommandsList["bc"] = {&CommandBc, &CommandBcHelp, DEBUGGER_COMMAND_BD_ATTRIBUTES};

    g_CommandsList["!epthook2"] = {&CommandEptHook2, &CommandEptHook2Help, DEBUGGER_COMMAND_EPTHOOK2_ATTRIBUTES};

    g_CommandsList["!cpuid"] = {&CommandCpuid, &CommandCpuidHelp, DEBUGGER_COMMAND_CPUID_ATTRIBUTES};

    g_CommandsList["!msrread"] = {&CommandMsrread, &CommandMsrreadHelp, DEBUGGER_COMMAND_MSRREAD_ATTRIBUTES};
    g_CommandsList["!msread"]  = {&CommandMsrread, &CommandMsrreadHelp, DEBUGGER_COMMAND_MSRREAD_ATTRIBUTES};

    g_CommandsList["!msrwrite"] = {&CommandMsrwrite, &CommandMsrwriteHelp, DEBUGGER_COMMAND_MSRWRITE_ATTRIBUTES};

    g_CommandsList["!tsc"] = {&CommandTsc, &CommandTscHelp, DEBUGGER_COMMAND_TSC_ATTRIBUTES};

    g_CommandsList["!pmc"] = {&CommandPmc, &CommandPmcHelp, DEBUGGER_COMMAND_PMC_ATTRIBUTES};

    g_CommandsList["!crwrite"] = {&CommandCrwrite, &CommandCrwriteHelp, DEBUGGER_COMMAND_CRWRITE_ATTRIBUTES};

    g_CommandsList["!dr"] = {&CommandDr, &CommandDrHelp, DEBUGGER_COMMAND_DR_ATTRIBUTES};

    g_CommandsList["!ioin"] = {&CommandIoin, &CommandIoinHelp, DEBUGGER_COMMAND_IOIN_ATTRIBUTES};

    g_CommandsList["!ioout"] = {&CommandIoout, &CommandIooutHelp, DEBUGGER_COMMAND_IOOUT_ATTRIBUTES};

    g_CommandsList["!exception"] = {&CommandException, &CommandExceptionHelp, DEBUGGER_COMMAND_EXCEPTION_ATTRIBUTES};

    g_CommandsList["!interrupt"] = {&CommandInterrupt, &CommandInterruptHelp, DEBUGGER_COMMAND_INTERRUPT_ATTRIBUTES};

    g_CommandsList["!syscall"]  = {&CommandSyscallAndSysret, &CommandSyscallHelp, DEBUGGER_COMMAND_SYSCALL_ATTRIBUTES};
    g_CommandsList["!syscall2"] = {&CommandSyscallAndSysret, &CommandSyscallHelp, DEBUGGER_COMMAND_SYSCALL_ATTRIBUTES};

    g_CommandsList["!sysret"]  = {&CommandSyscallAndSysret, &CommandSysretHelp, DEBUGGER_COMMAND_SYSRET_ATTRIBUTES};
    g_CommandsList["!sysret2"] = {&CommandSyscallAndSysret, &CommandSysretHelp, DEBUGGER_COMMAND_SYSRET_ATTRIBUTES};

    g_CommandsList["!hide"] = {&CommandHide, &CommandHideHelp, DEBUGGER_COMMAND_HIDE_ATTRIBUTES};

    g_CommandsList["!unhide"] = {&CommandUnhide, &CommandUnhideHelp, DEBUGGER_COMMAND_UNHIDE_ATTRIBUTES};

    g_CommandsList["!measure"] = {&CommandMeasure, &CommandMeasureHelp, DEBUGGER_COMMAND_MEASURE_ATTRIBUTES};

    g_CommandsList["lm"] = {&CommandLm, &CommandLmHelp, DEBUGGER_COMMAND_LM_ATTRIBUTES};

    g_CommandsList["p"]  = {&CommandP, &CommandPHelp, DEBUGGER_COMMAND_P_ATTRIBUTES};
    g_CommandsList["pr"] = {&CommandP, &CommandPHelp, DEBUGGER_COMMAND_P_ATTRIBUTES};

    g_CommandsList["t"]  = {&CommandT, &CommandTHelp, DEBUGGER_COMMAND_T_ATTRIBUTES};
    g_CommandsList["tr"] = {&CommandT, &CommandTHelp, DEBUGGER_COMMAND_T_ATTRIBUTES};

    g_CommandsList["i"]  = {&CommandI, &CommandIHelp, DEBUGGER_COMMAND_I_ATTRIBUTES};
    g_CommandsList["ir"] = {&CommandI, &CommandIHelp, DEBUGGER_COMMAND_I_ATTRIBUTES};

    g_CommandsList["db"]  = {&CommandReadMemoryAndDisassembler,
                             &CommandReadMemoryAndDisassemblerHelp,
                             DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["dc"]  = {&CommandReadMemoryAndDisassembler,
                             &CommandReadMemoryAndDisassemblerHelp,
                             DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["dd"]  = {&CommandReadMemoryAndDisassembler,
                             &CommandReadMemoryAndDisassemblerHelp,
                             DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["dq"]  = {&CommandReadMemoryAndDisassembler,
                             &CommandReadMemoryAndDisassemblerHelp,
                             DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["!db"] = {&CommandReadMemoryAndDisassembler,
                             &CommandReadMemoryAndDisassemblerHelp,
                             DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["!dc"] = {&CommandReadMemoryAndDisassembler,
                             &CommandReadMemoryAndDisassemblerHelp,
                             DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["!dd"] = {&CommandReadMemoryAndDisassembler,
                             &CommandReadMemoryAndDisassemblerHelp,
                             DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["!dq"] = {&CommandReadMemoryAndDisassembler,
                             &CommandReadMemoryAndDisassemblerHelp,
                             DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["!u"]  = {&CommandReadMemoryAndDisassembler,
                             &CommandReadMemoryAndDisassemblerHelp,
                             DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["u"]   = {&CommandReadMemoryAndDisassembler,
                             &CommandReadMemoryAndDisassemblerHelp,
                             DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["!u2"] = {&CommandReadMemoryAndDisassembler,
                             &CommandReadMemoryAndDisassemblerHelp,
                             DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandsList["u2"]  = {&CommandReadMemoryAndDisassembler,
                             &CommandReadMemoryAndDisassemblerHelp,
                             DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};

    g_CommandsList["eb"]  = {&CommandEditMemory, &CommandEditMemoryHelp, DEBUGGER_COMMAND_E_ATTRIBUTES};
    g_CommandsList["ed"]  = {&CommandEditMemory, &CommandEditMemoryHelp, DEBUGGER_COMMAND_E_ATTRIBUTES};
    g_CommandsList["eq"]  = {&CommandEditMemory, &CommandEditMemoryHelp, DEBUGGER_COMMAND_E_ATTRIBUTES};
    g_CommandsList["!eb"] = {&CommandEditMemory, &CommandEditMemoryHelp, DEBUGGER_COMMAND_E_ATTRIBUTES};
    g_CommandsList["!ed"] = {&CommandEditMemory, &CommandEditMemoryHelp, DEBUGGER_COMMAND_E_ATTRIBUTES};
    g_CommandsList["!eq"] = {&CommandEditMemory, &CommandEditMemoryHelp, DEBUGGER_COMMAND_E_ATTRIBUTES};

    g_CommandsList["sb"]  = {&CommandSearchMemory, &CommandSearchMemoryHelp, DEBUGGER_COMMAND_S_ATTRIBUTES};
    g_CommandsList["sd"]  = {&CommandSearchMemory, &CommandSearchMemoryHelp, DEBUGGER_COMMAND_S_ATTRIBUTES};
    g_CommandsList["sq"]  = {&CommandSearchMemory, &CommandSearchMemoryHelp, DEBUGGER_COMMAND_S_ATTRIBUTES};
    g_CommandsList["!sb"] = {&CommandSearchMemory, &CommandSearchMemoryHelp, DEBUGGER_COMMAND_S_ATTRIBUTES};
    g_CommandsList["!sd"] = {&CommandSearchMemory, &CommandSearchMemoryHelp, DEBUGGER_COMMAND_S_ATTRIBUTES};
    g_CommandsList["!sq"] = {&CommandSearchMemory, &CommandSearchMemoryHelp, DEBUGGER_COMMAND_S_ATTRIBUTES};

    g_CommandsList["r"] = {&CommandR, &CommandRHelp, DEBUGGER_COMMAND_R_ATTRIBUTES};

    g_CommandsList[".sympath"] = {&CommandSympath, &CommandSympathHelp, DEBUGGER_COMMAND_SYMPATH_ATTRIBUTES};
    g_CommandsList["sympath"]  = {&CommandSympath, &CommandSympathHelp, DEBUGGER_COMMAND_SYMPATH_ATTRIBUTES};

    g_CommandsList[".sym"] = {&CommandSym, &CommandSymHelp, DEBUGGER_COMMAND_SYM_ATTRIBUTES};
    g_CommandsList["sym"]  = {&CommandSym, &CommandSymHelp, DEBUGGER_COMMAND_SYM_ATTRIBUTES};

    g_CommandsList["x"] = {&CommandX, &CommandXHelp, DEBUGGER_COMMAND_X_ATTRIBUTES};

    g_CommandsList["prealloc"]    = {&CommandPrealloc, &CommandPreallocHelp, DEBUGGER_COMMAND_PREALLOC_ATTRIBUTES};
    g_CommandsList["preallocate"] = {&CommandPrealloc, &CommandPreallocHelp, DEBUGGER_COMMAND_PREALLOC_ATTRIBUTES};

    g_CommandsList["k"]  = {&CommandK, &CommandKHelp, DEBUGGER_COMMAND_K_ATTRIBUTES};
    g_CommandsList["kd"] = {&CommandK, &CommandKHelp, DEBUGGER_COMMAND_K_ATTRIBUTES};
    g_CommandsList["kq"] = {&CommandK, &CommandKHelp, DEBUGGER_COMMAND_K_ATTRIBUTES};

    g_CommandsList["dt"]  = {&CommandDtAndStruct, &CommandDtHelp, DEBUGGER_COMMAND_DT_ATTRIBUTES};
    g_CommandsList["!dt"] = {&CommandDtAndStruct, &CommandDtHelp, DEBUGGER_COMMAND_DT_ATTRIBUTES};

    g_CommandsList["struct"]    = {&CommandDtAndStruct, &CommandStructHelp, DEBUGGER_COMMAND_STRUCT_ATTRIBUTES};
    g_CommandsList["structure"] = {&CommandDtAndStruct, &CommandStructHelp, DEBUGGER_COMMAND_STRUCT_ATTRIBUTES};

    g_CommandsList[".pe"] = {&CommandPe, &CommandPeHelp, DEBUGGER_COMMAND_PE_ATTRIBUTES};
}
