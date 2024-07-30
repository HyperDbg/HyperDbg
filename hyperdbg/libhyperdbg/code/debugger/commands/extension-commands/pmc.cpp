/**
 * @file pmc.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !pmc commands
 * @details
 * @version 0.1
 * @date 2020-06-03
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of the !pmc command
 *
 * @return VOID
 */
VOID
CommandPmcHelp()
{
    ShowMessages("!pmc : monitors execution of rdpmc instructions.\n\n");

    ShowMessages("syntax : \t!pmc [pid ProcessId (hex)] [core CoreId (hex)] [imm IsImmediate (yesno)] "
                 "[sc EnableShortCircuiting (onoff)] [stage CallingStage (prepostall)] [buffer PreAllocatedBuffer (hex)] "
                 "[script { Script (string) }] [asm condition { Condition (assembly/hex) }] [asm code { Code (assembly/hex) }] "
                 "[output {OutputName (string)}]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !pmc\n");
    ShowMessages("\t\te.g : !pmc pid 400\n");
    ShowMessages("\t\te.g : !pmc core 2 pid 400\n");
    ShowMessages("\t\te.g : !pmc script { printf(\"RDPMC instruction called at: %%llx\\n\", @rip); }\n");
    ShowMessages("\t\te.g : !pmc asm code { nop; nop; nop }\n");
}

/**
 * @brief !pmc command handler
 *
 * @param CommandTokens
 *
 * @return VOID
 */
VOID
CommandPmc(vector<CommandToken> CommandTokens)
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
            PMC_INSTRUCTION_EXECUTION,
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
        CommandPmcHelp();

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
