/**
 * @file interpreter.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
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
extern DEBUGGING_STATE g_DebuggingState;
extern BOOLEAN         g_IsCommandListInitialized;
extern CommandType     g_CommandList;
extern BOOLEAN         g_LogOpened;
extern BOOLEAN         g_ExecutingScript;
extern BOOLEAN         g_IsConnectedToHyperDbgLocally;
extern BOOLEAN         g_IsConnectedToRemoteDebuggee;
extern BOOLEAN         g_IsRemoteDebuggerMessageReceived;
extern BOOLEAN         g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN         g_IsDebuggeeRunning;
extern string          g_ServerPort;
extern string          g_ServerIp;

/**
 * @brief Interpret commands
 *
 * @param Command The text of command
 * @return int returns return zero if it was successful or non-zero if there was
 * error
 */
int
HyperdbgInterpreter(const char * Command)
{
    string                CommandString(Command);
    BOOLEAN               HelpCommand = FALSE;
    CommandType::iterator Iterator;

    //
    // Check if it's the first command and whether the mapping of command is
    // initialized or not
    //
    if (!g_IsCommandListInitialized)
    {
        //
        // Initialize the mapping of functions
        //
        InitializeCommandsDictionary();

        g_IsCommandListInitialized = TRUE;
    }
    //
    // Save the command into log open file
    //
    if (g_LogOpened && !g_ExecutingScript)
    {
        LogopenSaveToFile("HyperDbg> ");
        LogopenSaveToFile(Command);
        LogopenSaveToFile("\n");
    }

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
    // Check and send remote command and also we check whether this
    // is a command that should be handled in this command or we can
    // send it to the remote computer, it is because in a remote connection
    // still some of the commands should be handled in the local HyperDbg
    //
    if (g_IsConnectedToRemoteDebuggee &&
        !(GetCommandAttributes(FirstCommand) &
          DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_REMOTE_CONNECTION))
    {
        //
        // It's a connection over network (VMI-Mode)
        //
        RemoteConnectionSendCommand(Command, strlen(Command) + 1);

        //
        // Indicate that we sent the command to the target system
        //
        return 2;
    }
    else if (g_IsSerialConnectedToRemoteDebuggee &&
             !(GetCommandAttributes(FirstCommand) &
               DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE))
    {
        //
        // It's a connection over serial (Debugger-Mode)
        //
        KdSendUserInputPacketToDebuggee(Command, strlen(Command) + 1);
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
            ShowMessages("Incorrect use of '%s'\n", FirstCommand.c_str());
            CommandHelpHelp();
            return 0;
        }
    }

    //
    // Start parsing commands
    //
    Iterator = g_CommandList.find(FirstCommand);

    if (Iterator == g_CommandList.end())
    {
        //
        //  Command doesn't exist
        //
        ShowMessages("Couldn't resolve error at '%s'\n", FirstCommand.c_str());
    }
    else
    {
        if (HelpCommand)
        {
            Iterator->second.CommandHelpFunction();
        }
        else
        {
            Iterator->second.CommandFunction(SplittedCommand, CommandString);
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

VOID
HyperdbgShowSignature()
{
    if (g_IsConnectedToRemoteDebuggee)
    {
        if (g_IsRemoteDebuggerMessageReceived)
        {
            ShowMessages("HyperDbg [%s:%s] >", g_ServerIp.c_str(), g_ServerPort.c_str());
        }
    }
    else if (g_DebuggingState.IsAttachedToUsermodeProcess)
    {
        ShowMessages("HyperDbg (%x:%x) >", g_DebuggingState.ConnectedProcessId, g_DebuggingState.ConnectedThreadId);
    }
    else
    {
        ShowMessages("HyperDbg> ");
    }
}

/**
 * @brief Get Command Attributes
 *
 * @param FirstCommand just the first word of command (without other parameters)
 * @return BOOLEAN Mask of the command's attributes
 */
UINT64
GetCommandAttributes(string FirstCommand)
{
    CommandType::iterator Iterator;

    //
    // Some commands should not be passed to the remote system
    // and instead should be handled in the current debugger
    //

    Iterator = g_CommandList.find(FirstCommand);

    if (Iterator == g_CommandList.end())
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

VOID
InitializeCommandsDictionary()
{
    g_CommandList[".help"] = {NULL, &CommandHelpHelp, DEBUGGER_COMMAND_HELP_ATTRIBUTES};
    g_CommandList[".hh"]   = {NULL, &CommandHelpHelp, DEBUGGER_COMMAND_HELP_ATTRIBUTES};
    g_CommandList["help"]  = {NULL, &CommandHelpHelp, DEBUGGER_COMMAND_HELP_ATTRIBUTES};

    g_CommandList["clear"] = {&CommandClearScreen, &CommandClearScreenHelp, DEBUGGER_COMMAND_CLEAR_ATTRIBUTES};
    g_CommandList[".cls"]  = {&CommandClearScreen, &CommandClearScreenHelp, DEBUGGER_COMMAND_CLEAR_ATTRIBUTES};
    g_CommandList["cls"]   = {&CommandClearScreen, &CommandClearScreenHelp, DEBUGGER_COMMAND_CLEAR_ATTRIBUTES};

    g_CommandList[".connect"] = {&CommandConnect, &CommandConnectHelp, DEBUGGER_COMMAND_CONNECT_ATTRIBUTES};
    g_CommandList["connect"]  = {&CommandConnect, &CommandConnectHelp, DEBUGGER_COMMAND_CONNECT_ATTRIBUTES};

    g_CommandList[".listen"] = {&CommandListen, &CommandListenHelp, DEBUGGER_COMMAND_LISTEN_ATTRIBUTES};
    g_CommandList["listen"]  = {&CommandListen, &CommandListenHelp, DEBUGGER_COMMAND_LISTEN_ATTRIBUTES};

    g_CommandList["g"]  = {&CommandG, &CommandGHelp, DEBUGGER_COMMAND_G_ATTRIBUTES};
    g_CommandList["go"] = {&CommandG, &CommandGHelp, DEBUGGER_COMMAND_G_ATTRIBUTES};

    g_CommandList[".attach"] = {&CommandAttach, &CommandAttachHelp, DEBUGGER_COMMAND_ATTACH_ATTRIBUTES};
    g_CommandList["attach"]  = {&CommandAttach, &CommandAttachHelp, DEBUGGER_COMMAND_ATTACH_ATTRIBUTES};

    g_CommandList[".detach"] = {&CommandDetach, &CommandDetachHelp, DEBUGGER_COMMAND_DETACH_ATTRIBUTES};
    g_CommandList["detach"]  = {&CommandDetach, &CommandDetachHelp, DEBUGGER_COMMAND_DETACH_ATTRIBUTES};

    g_CommandList[".process"] = {&CommandProcess, &CommandProcessHelp, DEBUGGER_COMMAND_PROCESS_ATTRIBUTES};

    g_CommandList["sleep"] = {&CommandSleep, &CommandSleepHelp, DEBUGGER_COMMAND_SLEEP_ATTRIBUTES};

    g_CommandList["event"]  = {&CommandEvents, &CommandEventsHelp, DEBUGGER_COMMAND_EVENTS_ATTRIBUTES};
    g_CommandList["events"] = {&CommandEvents, &CommandEventsHelp, DEBUGGER_COMMAND_EVENTS_ATTRIBUTES};

    g_CommandList["setting"]  = {&CommandSettings, &CommandSettingsHelp, DEBUGGER_COMMAND_SETTINGS_ATTRIBUTES};
    g_CommandList["settings"] = {&CommandSettings, &CommandSettingsHelp, DEBUGGER_COMMAND_SETTINGS_ATTRIBUTES};

    g_CommandList[".disconnect"] = {&CommandDisconnect, &CommandDisconnectHelp, DEBUGGER_COMMAND_DISCONNECT_ATTRIBUTES};
    g_CommandList["disconnect"]  = {&CommandDisconnect, &CommandDisconnectHelp, DEBUGGER_COMMAND_DISCONNECT_ATTRIBUTES};

    g_CommandList[".debug"] = {&CommandDebug, &CommandDebugHelp, DEBUGGER_COMMAND_DEBUG_ATTRIBUTES};
    g_CommandList["debug"]  = {&CommandDebug, &CommandDebugHelp, DEBUGGER_COMMAND_DEBUG_ATTRIBUTES};

    g_CommandList[".status"] = {&CommandStatus, &CommandStatusHelp, DEBUGGER_COMMAND_DOT_STATUS_ATTRIBUTES};
    g_CommandList["status"]  = {&CommandStatus, &CommandStatusHelp, DEBUGGER_COMMAND_STATUS_ATTRIBUTES};

    g_CommandList["load"] = {&CommandLoad, &CommandLoadHelp, DEBUGGER_COMMAND_LOAD_ATTRIBUTES};

    g_CommandList["exit"]  = {&CommandExit, &CommandExitHelp, DEBUGGER_COMMAND_EXIT_ATTRIBUTES};
    g_CommandList[".exit"] = {&CommandExit, &CommandExitHelp, DEBUGGER_COMMAND_EXIT_ATTRIBUTES};

    g_CommandList["flush"] = {&CommandFlush, &CommandFlushHelp, DEBUGGER_COMMAND_FLUSH_ATTRIBUTES};

    g_CommandList["pause"] = {&CommandPause, &CommandPauseHelp, DEBUGGER_COMMAND_PAUSE_ATTRIBUTES};

    g_CommandList["unload"] = {&CommandUnload, &CommandUnloadHelp, DEBUGGER_COMMAND_UNLOAD_ATTRIBUTES};

    g_CommandList[".script"] = {&CommandScript, &CommandScriptHelp, DEBUGGER_COMMAND_SCRIPT_ATTRIBUTES};

    g_CommandList["output"] = {&CommandOutput, &CommandOutputHelp, DEBUGGER_COMMAND_OUTPUT_ATTRIBUTES};

    g_CommandList["print"] = {&CommandPrint, &CommandPrintHelp, DEBUGGER_COMMAND_PRINT_ATTRIBUTES};

    g_CommandList["?"]        = {&CommandEval, &CommandEvalHelp, DEBUGGER_COMMAND_EVAL_ATTRIBUTES};
    g_CommandList["eval"]     = {&CommandEval, &CommandEvalHelp, DEBUGGER_COMMAND_EVAL_ATTRIBUTES};
    g_CommandList["evaluate"] = {&CommandEval, &CommandEvalHelp, DEBUGGER_COMMAND_EVAL_ATTRIBUTES};

    g_CommandList[".logopen"] = {&CommandLogopen, &CommandLogopenHelp, DEBUGGER_COMMAND_LOGOPEN_ATTRIBUTES};

    g_CommandList[".logclose"] = {&CommandLogclose, &CommandLogcloseHelp, DEBUGGER_COMMAND_LOGCLOSE_ATTRIBUTES};

    g_CommandList["test"] = {&CommandTest, &CommandTestHelp, DEBUGGER_COMMAND_TEST_ATTRIBUTES};

    g_CommandList["cpu"] = {&CommandCpu, &CommandCpuHelp, DEBUGGER_COMMAND_CPU_ATTRIBUTES};

    g_CommandList["wrmsr"] = {&CommandWrmsr, &CommandWrmsrHelp, DEBUGGER_COMMAND_WRMSR_ATTRIBUTES};

    g_CommandList["rdmsr"] = {&CommandRdmsr, &CommandRdmsrHelp, DEBUGGER_COMMAND_RDMSR_ATTRIBUTES};

    g_CommandList["!va2pa"] = {&CommandVa2pa, &CommandVa2paHelp, DEBUGGER_COMMAND_VA2PA_ATTRIBUTES};

    g_CommandList["!pa2va"] = {&CommandPa2va, &CommandPa2vaHelp, DEBUGGER_COMMAND_PA2VA_ATTRIBUTES};

    g_CommandList[".formats"] = {&CommandFormats, &CommandFormatsHelp, DEBUGGER_COMMAND_FORMATS_ATTRIBUTES};
    g_CommandList[".format"]  = {&CommandFormats, &CommandFormatsHelp, DEBUGGER_COMMAND_FORMATS_ATTRIBUTES};

    g_CommandList["!pte"] = {&CommandPte, &CommandPteHelp, DEBUGGER_COMMAND_PTE_ATTRIBUTES};

    g_CommandList["~"]    = {&CommandCore, &CommandCoreHelp, DEBUGGER_COMMAND_CORE_ATTRIBUTES};
    g_CommandList["core"] = {&CommandCore, &CommandCoreHelp, DEBUGGER_COMMAND_CORE_ATTRIBUTES};

    g_CommandList["!monitor"] = {&CommandMonitor, &CommandMonitorHelp, DEBUGGER_COMMAND_MONITOR_ATTRIBUTES};

    g_CommandList["!vmcall"] = {&CommandVmcall, &CommandVmcallHelp, DEBUGGER_COMMAND_VMCALL_ATTRIBUTES};

    g_CommandList["!epthook"] = {&CommandEptHook, &CommandEptHookHelp, DEBUGGER_COMMAND_EPTHOOK_ATTRIBUTES};
    g_CommandList["bh"]       = {&CommandEptHook, &CommandEptHookHelp, DEBUGGER_COMMAND_EPTHOOK_ATTRIBUTES};

    g_CommandList["bp"] = {&CommandBp, &CommandBpHelp, DEBUGGER_COMMAND_BP_ATTRIBUTES};

    g_CommandList["bl"] = {&CommandBl, &CommandBlHelp, DEBUGGER_COMMAND_BD_ATTRIBUTES};

    g_CommandList["be"] = {&CommandBe, &CommandBeHelp, DEBUGGER_COMMAND_BD_ATTRIBUTES};

    g_CommandList["bd"] = {&CommandBd, &CommandBdHelp, DEBUGGER_COMMAND_BD_ATTRIBUTES};

    g_CommandList["bc"] = {&CommandBc, &CommandBcHelp, DEBUGGER_COMMAND_BD_ATTRIBUTES};

    g_CommandList["!epthook2"] = {&CommandEptHook2, &CommandEptHook2Help, DEBUGGER_COMMAND_EPTHOOK2_ATTRIBUTES};

    g_CommandList["!cpuid"] = {&CommandCpuid, &CommandCpuidHelp, DEBUGGER_COMMAND_CPUID_ATTRIBUTES};

    g_CommandList["!msrread"] = {&CommandMsrread, &CommandMsrreadHelp, DEBUGGER_COMMAND_MSRREAD_ATTRIBUTES};

    g_CommandList["!msrwrite"] = {&CommandMsrwrite, &CommandMsrwriteHelp, DEBUGGER_COMMAND_MSRWRITE_ATTRIBUTES};

    g_CommandList["!tsc"] = {&CommandTsc, &CommandTscHelp, DEBUGGER_COMMAND_TSC_ATTRIBUTES};

    g_CommandList["!pmc"] = {&CommandPmc, &CommandPmcHelp, DEBUGGER_COMMAND_PMC_ATTRIBUTES};

    g_CommandList["!dr"] = {&CommandDr, &CommandDrHelp, DEBUGGER_COMMAND_DR_ATTRIBUTES};

    g_CommandList["!ioin"] = {&CommandIoin, &CommandIoinHelp, DEBUGGER_COMMAND_IOIN_ATTRIBUTES};

    g_CommandList["!ioout"] = {&CommandIoout, &CommandIooutHelp, DEBUGGER_COMMAND_IOOUT_ATTRIBUTES};

    g_CommandList["!exception"] = {&CommandException, &CommandExceptionHelp, DEBUGGER_COMMAND_EXCEPTION_ATTRIBUTES};

    g_CommandList["!interrupt"] = {&CommandInterrupt, &CommandInterruptHelp, DEBUGGER_COMMAND_INTERRUPT_ATTRIBUTES};

    g_CommandList["!syscall"] = {&CommandSyscallAndSysret, &CommandSyscallHelp, DEBUGGER_COMMAND_SYSCALL_ATTRIBUTES};

    g_CommandList["!sysret"] = {&CommandSyscallAndSysret, &CommandSysretHelp, DEBUGGER_COMMAND_SYSRET_ATTRIBUTES};

    g_CommandList["!hide"] = {&CommandHide, &CommandHideHelp, DEBUGGER_COMMAND_HIDE_ATTRIBUTES};

    g_CommandList["!unhide"] = {&CommandUnhide, &CommandUnhideHelp, DEBUGGER_COMMAND_UNHIDE_ATTRIBUTES};

    g_CommandList["!measure"] = {&CommandMeasure, &CommandMeasureHelp, DEBUGGER_COMMAND_MEASURE_ATTRIBUTES};

    g_CommandList["lm"] = {&CommandLm, &CommandLmHelp, DEBUGGER_COMMAND_LM_ATTRIBUTES};

    g_CommandList["p"]  = {&CommandP, &CommandPHelp, DEBUGGER_COMMAND_P_ATTRIBUTES};
    g_CommandList["pr"] = {&CommandP, &CommandPHelp, DEBUGGER_COMMAND_P_ATTRIBUTES};

    g_CommandList["t"]  = {&CommandT, &CommandTHelp, DEBUGGER_COMMAND_T_ATTRIBUTES};
    g_CommandList["tr"] = {&CommandT, &CommandTHelp, DEBUGGER_COMMAND_T_ATTRIBUTES};

    g_CommandList["i"]  = {&CommandI, &CommandIHelp, DEBUGGER_COMMAND_I_ATTRIBUTES};
    g_CommandList["ir"] = {&CommandI, &CommandIHelp, DEBUGGER_COMMAND_I_ATTRIBUTES};

    g_CommandList["db"]  = {&CommandReadMemoryAndDisassembler,
                           &CommandReadMemoryAndDisassemblerHelp,
                           DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandList["dc"]  = {&CommandReadMemoryAndDisassembler,
                           &CommandReadMemoryAndDisassemblerHelp,
                           DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandList["dd"]  = {&CommandReadMemoryAndDisassembler,
                           &CommandReadMemoryAndDisassemblerHelp,
                           DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandList["dq"]  = {&CommandReadMemoryAndDisassembler,
                           &CommandReadMemoryAndDisassemblerHelp,
                           DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandList["!db"] = {&CommandReadMemoryAndDisassembler,
                            &CommandReadMemoryAndDisassemblerHelp,
                            DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandList["!dc"] = {&CommandReadMemoryAndDisassembler,
                            &CommandReadMemoryAndDisassemblerHelp,
                            DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandList["!dd"] = {&CommandReadMemoryAndDisassembler,
                            &CommandReadMemoryAndDisassemblerHelp,
                            DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandList["!dq"] = {&CommandReadMemoryAndDisassembler,
                            &CommandReadMemoryAndDisassemblerHelp,
                            DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandList["!u"]  = {&CommandReadMemoryAndDisassembler,
                           &CommandReadMemoryAndDisassemblerHelp,
                           DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandList["u"]   = {&CommandReadMemoryAndDisassembler,
                          &CommandReadMemoryAndDisassemblerHelp,
                          DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandList["!u2"] = {&CommandReadMemoryAndDisassembler,
                            &CommandReadMemoryAndDisassemblerHelp,
                            DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};
    g_CommandList["u2"]  = {&CommandReadMemoryAndDisassembler,
                           &CommandReadMemoryAndDisassemblerHelp,
                           DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES};

    g_CommandList["eb"]  = {&CommandEditMemory, &CommandEditMemoryHelp, DEBUGGER_COMMAND_E_ATTRIBUTES};
    g_CommandList["ed"]  = {&CommandEditMemory, &CommandEditMemoryHelp, DEBUGGER_COMMAND_E_ATTRIBUTES};
    g_CommandList["eq"]  = {&CommandEditMemory, &CommandEditMemoryHelp, DEBUGGER_COMMAND_E_ATTRIBUTES};
    g_CommandList["!eb"] = {&CommandEditMemory, &CommandEditMemoryHelp, DEBUGGER_COMMAND_E_ATTRIBUTES};
    g_CommandList["!ed"] = {&CommandEditMemory, &CommandEditMemoryHelp, DEBUGGER_COMMAND_E_ATTRIBUTES};
    g_CommandList["!eq"] = {&CommandEditMemory, &CommandEditMemoryHelp, DEBUGGER_COMMAND_E_ATTRIBUTES};

    g_CommandList["sb"]  = {&CommandSearchMemory, &CommandSearchMemoryHelp, DEBUGGER_COMMAND_S_ATTRIBUTES};
    g_CommandList["sd"]  = {&CommandSearchMemory, &CommandSearchMemoryHelp, DEBUGGER_COMMAND_S_ATTRIBUTES};
    g_CommandList["sq"]  = {&CommandSearchMemory, &CommandSearchMemoryHelp, DEBUGGER_COMMAND_S_ATTRIBUTES};
    g_CommandList["!sb"] = {&CommandSearchMemory, &CommandSearchMemoryHelp, DEBUGGER_COMMAND_S_ATTRIBUTES};
    g_CommandList["!sd"] = {&CommandSearchMemory, &CommandSearchMemoryHelp, DEBUGGER_COMMAND_S_ATTRIBUTES};
    g_CommandList["!sq"] = {&CommandSearchMemory, &CommandSearchMemoryHelp, DEBUGGER_COMMAND_S_ATTRIBUTES};

    g_CommandList["r"] = {&CommandR, &CommandRHelp, DEBUGGER_COMMAND_R_ATTRIBUTES};
}
