/**
 * @file mode.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !mode commands
 * @details
 * @version 0.6
 * @date 2023-09-13
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of the !mode command
 *
 * @return VOID
 */
VOID
CommandModeHelp()
{
    ShowMessages("!mode : traps the execution of user-mode instructions.\n\n");

    ShowMessages("syntax : \t!mode [pid ProcessId (hex)] [core CoreId (hex)] [imm IsImmediate (yesno)] "
                 "[sc EnableShortCircuiting (onoff)] [stage CallingStage (prepostall)] [buffer PreAllocatedBuffer (hex)] "
                 "[script { Script (string) }] [condition { Condition (hex) }] [code { Code (hex) }]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !mode\n");
    ShowMessages("\t\te.g : !mode pid 1c0\n");
    ShowMessages("\t\te.g : !mode core 2 pid 400\n");
}

/**
 * @brief !mode command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandMode(vector<string> SplittedCommand, string Command)
{
    PDEBUGGER_GENERAL_EVENT_DETAIL     Event                 = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionBreakToDebugger = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionCustomCode      = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionScript          = NULL;
    UINT32                             EventLength;
    UINT32                             ActionBreakToDebuggerLength = 0;
    UINT32                             ActionCustomCodeLength      = 0;
    UINT32                             ActionScriptLength          = 0;
    vector<string>                     SplittedCommandCaseSensitive {Split(Command, ' ')};
    DEBUGGER_EVENT_PARSING_ERROR_CAUSE EventParsingErrorCause;

    //
    // Interpret and fill the general event and action fields
    //
    //
    if (!InterpretGeneralEventAndActionsFields(
            &SplittedCommand,
            &SplittedCommandCaseSensitive,
            TRAP_EXECUTION_USER_MODE,
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

    //
    // Check for size
    //
    if (SplittedCommand.size() > 1)
    {
        ShowMessages("incorrect use of the '!mode'\n");
        CommandModeHelp();

        FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
        return;
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
