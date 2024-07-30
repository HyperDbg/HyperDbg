/**
 * @file vmcall.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !vmcall command
 * @details
 * @version 0.1
 * @date 2020-07-03
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of the !vmcall command
 *
 * @return VOID
 */
VOID
CommandVmcallHelp()
{
    ShowMessages("!vmcall : monitors execution of VMCALL instruction.\n\n");

    ShowMessages("syntax : \t!vmcall [pid ProcessId (hex)] [core CoreId (hex)] [imm IsImmediate (yesno)] "
                 "[sc EnableShortCircuiting (onoff)] [stage CallingStage (prepostall)] [buffer PreAllocatedBuffer (hex)] "
                 "[script { Script (string) }] [asm condition { Condition (assembly/hex) }] [asm code { Code (assembly/hex) }] "
                 "[output {OutputName (string)}]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !vmcall\n");
    ShowMessages("\t\te.g : !vmcall pid 400\n");
    ShowMessages("\t\te.g : !vmcall core 2 pid 400\n");
    ShowMessages("\t\te.g : !vmcall script { printf(\"VMCALL executed with context: %%llx\\n\", $context); }\n");
    ShowMessages("\t\te.g : !vmcall asm code { nop; nop; nop }\n");
}

/**
 * @brief !vmcall command handler
 *
 * @param CommandTokens
 *
 * @return VOID
 */
VOID
CommandVmcall(vector<CommandToken> CommandTokens)
{
    PDEBUGGER_GENERAL_EVENT_DETAIL     Event                 = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionBreakToDebugger = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionCustomCode      = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionScript          = NULL;
    UINT32                             EventLength;
    UINT32                             ActionBreakToDebuggerLength = 0;
    UINT32                             ActionCustomCodeLength      = 0;
    UINT32                             ActionScriptLength          = 0;
    DEBUGGER_EVENT_PARSING_ERROR_CAUSE EventParsingErrorCause;

    //
    // Interpret and fill the general event and action fields
    //
    //
    if (!InterpretGeneralEventAndActionsFields(
            &CommandTokens,
            VMCALL_INSTRUCTION_EXECUTION,
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
    if (CommandTokens.size() > 1)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandVmcallHelp();

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
