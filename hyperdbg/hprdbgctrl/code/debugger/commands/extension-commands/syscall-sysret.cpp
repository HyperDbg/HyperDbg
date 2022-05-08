/**
 * @file syscall-sysret.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !syscall and !sysret commands
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of !syscall command
 *
 * @return VOID
 */
VOID
CommandSyscallHelp()
{
    ShowMessages("!syscall : monitors and hooks all execution of syscall "
                 "instructions (by accessing memory and checking for instructions).\n\n");
    ShowMessages("!syscall2 : monitors and hooks all execution of syscall "
                 "instructions (by emulating all #UDs).\n\n");

    ShowMessages("syntax : \t!syscall [SyscallNumber (hex)] [pid ProcessId (hex)] "
                 "[core CoreId (hex)] [imm IsImmediate (yesno)] [buffer PreAllocatedBuffer (hex)] "
                 "[script { Script (string) }] [condition { Condition (hex) }] [code { Code (hex) }]\n");
    ShowMessages("syntax : \t!syscall2 [SyscallNumber (hex)] [pid ProcessId (hex)] "
                 "[core CoreId (hex)] [imm IsImmediate (yesno)] [buffer PreAllocatedBuffer (hex)] "
                 "[script { Script (string) }] [condition { Condition (hex) }] [code { Code (hex) }]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !syscall\n");
    ShowMessages("\t\te.g : !syscall2\n");
    ShowMessages("\t\te.g : !syscall 0x55\n");
    ShowMessages("\t\te.g : !syscall2 0x55\n");
    ShowMessages("\t\te.g : !syscall 0x55 pid 400\n");
    ShowMessages("\t\te.g : !syscall 0x55 core 2 pid 400\n");
    ShowMessages("\t\te.g : !syscall2 0x55 core 2 pid 400\n");
}

/**
 * @brief help of !sysret command
 *
 * @return VOID
 */
VOID
CommandSysretHelp()
{
    ShowMessages("!sysret : monitors and hooks all execution of sysret "
                 "instructions (by accessing memory and checking for instructions).\n\n");
    ShowMessages("!sysret2 : monitors and hooks all execution of sysret "
                 "instructions (by emulating all #UDs).\n\n");

    ShowMessages("syntax : \t!sysret [pid ProcessId (hex)] [core CoreId (hex)] "
                 "[imm IsImmediate (yesno)] [buffer PreAllocatedBuffer (hex)] "
                 "[script { Script (string) }] [condition { Condition (hex) }] "
                 "[code { Code (hex) }]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !sysret\n");
    ShowMessages("\t\te.g : !sysret2\n");
    ShowMessages("\t\te.g : !sysret pid 400\n");
    ShowMessages("\t\te.g : !sysret2 pid 400\n");
    ShowMessages("\t\te.g : !sysret core 2 pid 400\n");
    ShowMessages("\t\te.g : !sysret2 core 2 pid 400\n");
}

/**
 * @brief !syscall, !syscall2 and !sysret, !sysret2 commands handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandSyscallAndSysret(vector<string> SplittedCommand, string Command)
{
    PDEBUGGER_GENERAL_EVENT_DETAIL     Event                 = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionBreakToDebugger = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionCustomCode      = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionScript          = NULL;
    UINT32                             EventLength;
    UINT32                             ActionBreakToDebuggerLength = 0;
    UINT32                             ActionCustomCodeLength      = 0;
    UINT32                             ActionScriptLength          = 0;
    UINT64                             SpecialTarget               = DEBUGGER_EVENT_SYSCALL_ALL_SYSRET_OR_SYSCALLS;
    BOOLEAN                            GetSyscallNumber            = FALSE;
    vector<string>                     SplittedCommandCaseSensitive {Split(Command, ' ')};
    DEBUGGER_EVENT_PARSING_ERROR_CAUSE EventParsingErrorCause;
    string                             Cmd;

    //
    // Interpret and fill the general event and action fields
    //
    //
    Cmd = SplittedCommand.at(0);
    if (!Cmd.compare("!syscall") || !Cmd.compare("!syscall2"))
    {
        if (!InterpretGeneralEventAndActionsFields(
                &SplittedCommand,
                &SplittedCommandCaseSensitive,
                SYSCALL_HOOK_EFER_SYSCALL,
                &Event,
                &EventLength,
                &ActionBreakToDebugger,
                &ActionBreakToDebuggerLength,
                &ActionCustomCode,
                &ActionCustomCodeLength,
                &ActionScript,
                &ActionScriptLength,
                &EventParsingErrorCause))
        {
            return;
        }
    }
    else
    {
        if (!InterpretGeneralEventAndActionsFields(
                &SplittedCommand,
                &SplittedCommandCaseSensitive,
                SYSCALL_HOOK_EFER_SYSRET,
                &Event,
                &EventLength,
                &ActionBreakToDebugger,
                &ActionBreakToDebuggerLength,
                &ActionCustomCode,
                &ActionCustomCodeLength,
                &ActionScript,
                &ActionScriptLength,
                &EventParsingErrorCause))
        {
            return;
        }
    }

    //
    // Interpret command specific details (if any)
    //

    //
    // Currently we do not support any extra argument for !sysret command
    // it is because we don't know how to find syscall number in sysret
    // and we don't wanna deal with dynamic mapping of rcx (user stack)
    // in vmx-root
    //
    if (!Cmd.compare("!syscall") || !Cmd.compare("!syscall2"))
    {
        for (auto Section : SplittedCommand)
        {
            if (!Section.compare("!syscall") ||
                !Section.compare("!syscall2") ||
                !Section.compare("!sysret") ||
                !Section.compare("!sysret2"))
            {
                continue;
            }

            else if (!GetSyscallNumber)
            {
                //
                // It's probably a syscall address
                //
                if (!ConvertStringToUInt64(Section, &SpecialTarget))
                {
                    //
                    // Unkonwn parameter
                    //
                    ShowMessages("unknown parameter '%s'\n\n", Section.c_str());

                    if (!Cmd.compare("!syscall") || !Cmd.compare("!syscall2"))
                    {
                        CommandSyscallHelp();
                    }
                    else
                    {
                        CommandSysretHelp();
                    }

                    FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
                    return;
                }
                else
                {
                    GetSyscallNumber = TRUE;
                }
            }
            else
            {
                //
                // Unkonwn parameter
                //
                ShowMessages("unknown parameter '%s'\n\n", Section.c_str());

                if (!Cmd.compare("!syscall") || !Cmd.compare("!syscall2"))
                {
                    CommandSyscallHelp();
                }
                else
                {
                    CommandSysretHelp();
                }

                FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
                return;
            }
        }

        //
        // Set the target syscall
        //
        Event->OptionalParam1 = SpecialTarget;
    }

    //
    // Set whether it's !syscall or !syscall2 or !sysret or !sysret2
    //
    if (!Cmd.compare("!syscall2") || !Cmd.compare("!sysret2"))
    {
        //
        // It's a !syscall2 or !sysret2
        //
        Event->OptionalParam2 = DEBUGGER_EVENT_SYSCALL_SYSRET_HANDLE_ALL_UD;
    }
    else
    {
        //
        // It's a !syscall or !sysret
        //
        Event->OptionalParam2 = DEBUGGER_EVENT_SYSCALL_SYSRET_SAFE_ACCESS_MEMORY;
    }

    //
    // Send the ioctl to the kernel for event registration
    //
    if (!SendEventToKernel(Event, EventLength))
    {
        //
        // There was an error, probably the handle was not initialized
        // we have to free the Action before exit, it is because, we
        // already freed the Event and string buffers
        //

        FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
        return;
    }

    //
    // Add the event to the kernel
    //
    if (!RegisterActionToEvent(Event,
                               ActionBreakToDebugger,
                               ActionBreakToDebuggerLength,
                               ActionCustomCode,
                               ActionCustomCodeLength,
                               ActionScript,
                               ActionScriptLength))
    {
        //
        // There was an error
        //

        FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
        return;
    }
}
