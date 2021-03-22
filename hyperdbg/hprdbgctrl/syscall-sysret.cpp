/**
 * @file syscall-sysret.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
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
    ShowMessages("!syscall : Monitors and hooks all execution of syscall "
                 "instructions.\n\n");
    ShowMessages("syntax : \t!syscall [syscall num (hex)] core [core index "
                 "(hex value)] pid [process id (hex value)] condition {[assembly "
                 "in hex]} code {[assembly in hex]} buffer [pre-require buffer - "
                 "(hex value)] \n");

    ShowMessages("\t\te.g : !syscall\n");
    ShowMessages("\t\te.g : !syscall 0x55\n");
    ShowMessages("\t\te.g : !syscall 0x55 pid 400\n");
    ShowMessages("\t\te.g : !syscall 0x55 core 2 pid 400\n");
}

/**
 * @brief help of !sysret command
 *
 * @return VOID
 */
VOID
CommandSysretHelp()
{
    ShowMessages("!sysret : Monitors and hooks all execution of sysret "
                 "instructions.\n\n");
    ShowMessages("syntax : \t!sysret core [core index "
                 "(hex value)] pid [process id (hex value)] condition {[assembly "
                 "in hex]} code {[assembly in hex]} buffer [pre-require buffer - "
                 "(hex value)] \n");

    ShowMessages("\t\te.g : !sysret\n");
    ShowMessages("\t\te.g : !sysret pid 400\n");
    ShowMessages("\t\te.g : !sysret core 2 pid 400\n");
}

/**
 * @brief !syscall and !sysret commands handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandSyscallAndSysret(vector<string> SplittedCommand, string Command)
{
    PDEBUGGER_GENERAL_EVENT_DETAIL Event                 = NULL;
    PDEBUGGER_GENERAL_ACTION       ActionBreakToDebugger = NULL;
    PDEBUGGER_GENERAL_ACTION       ActionCustomCode      = NULL;
    PDEBUGGER_GENERAL_ACTION       ActionScript          = NULL;
    UINT32                         EventLength;
    UINT32                         ActionBreakToDebuggerLength = 0;
    UINT32                         ActionCustomCodeLength      = 0;
    UINT32                         ActionScriptLength          = 0;
    UINT64                         SpecialTarget               = DEBUGGER_EVENT_SYSCALL_ALL_SYSRET_OR_SYSCALLS;
    BOOLEAN                        GetSyscallNumber            = FALSE;

    //
    // Interpret and fill the general event and action fields
    //
    //
    if (!SplittedCommand.at(0).compare("!syscall"))
    {
        if (!InterpretGeneralEventAndActionsFields(
                &SplittedCommand,
                SYSCALL_HOOK_EFER_SYSCALL,
                &Event,
                &EventLength,
                &ActionBreakToDebugger,
                &ActionBreakToDebuggerLength,
                &ActionCustomCode,
                &ActionCustomCodeLength,
                &ActionScript,
                &ActionScriptLength))
        {
            CommandSyscallHelp();
            return;
        }
    }
    else
    {
        if (!InterpretGeneralEventAndActionsFields(
                &SplittedCommand,
                SYSCALL_HOOK_EFER_SYSRET,
                &Event,
                &EventLength,
                &ActionBreakToDebugger,
                &ActionBreakToDebuggerLength,
                &ActionCustomCode,
                &ActionCustomCodeLength,
                &ActionScript,
                &ActionScriptLength))
        {
            CommandSysretHelp();
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
    if (!SplittedCommand.at(0).compare("!syscall"))
    {
        for (auto Section : SplittedCommand)
        {
            if (!Section.compare("!syscall") || !Section.compare("!sysret"))
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
                    ShowMessages("Unknown parameter '%s'\n\n", Section.c_str());
                    if (!SplittedCommand.at(0).compare("!syscall"))
                    {
                        CommandSyscallHelp();
                    }
                    else
                    {
                        CommandSysretHelp();
                    }
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
                ShowMessages("Unknown parameter '%s'\n\n", Section.c_str());
                if (!SplittedCommand.at(0).compare("!syscall"))
                {
                    CommandSyscallHelp();
                }
                else
                {
                    CommandSysretHelp();
                }
                return;
            }
        }

        //
        // Set the target syscall
        //
        Event->OptionalParam1 = SpecialTarget;
    }

    //
    // Send the ioctl to the kernel for event registeration
    //
    if (!SendEventToKernel(Event, EventLength))
    {
        //
        // There was an error, probably the handle was not initialized
        // we have to free the Action before exit, it is because, we
        // already freed the Event and string buffers
        //
        if (ActionBreakToDebugger != NULL)
        {
            free(ActionBreakToDebugger);
        }
        if (ActionCustomCode != NULL)
        {
            free(ActionCustomCode);
        }
        if (ActionScript != NULL)
        {
            free(ActionScript);
        }
        return;
    }

    //
    // Add the event to the kernel
    //
    if (!RegisterActionToEvent(ActionBreakToDebugger, ActionBreakToDebuggerLength, ActionCustomCode, ActionCustomCodeLength, ActionScript, ActionScriptLength))
    {
        //
        // There was an error
        //
        return;
    }
}
