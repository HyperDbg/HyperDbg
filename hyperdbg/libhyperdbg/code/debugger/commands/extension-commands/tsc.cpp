/**
 * @file tsc.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !tsc commands
 * @details
 * @version 0.1
 * @date 2020-06-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of the !tsc command
 *
 * @return VOID
 */
VOID
CommandTscHelp()
{
    ShowMessages("!tsc : monitors execution of rdtsc/rdtscp instructions.\n\n");

    ShowMessages("syntax : \t!tsc [pid ProcessId (hex)] [core CoreId (hex)] [imm IsImmediate (yesno)] "
                 "[sc EnableShortCircuiting (onoff)] [stage CallingStage (prepostall)] [buffer PreAllocatedBuffer (hex)] "
                 "[script { Script (string) }] [asm condition { Condition (assembly/hex) }] [asm code { Code (assembly/hex) }] "
                 "[output {OutputName (string)}]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !tsc\n");
    ShowMessages("\t\te.g : !tsc pid 400\n");
    ShowMessages("\t\te.g : !tsc core 2 pid 400\n");
    ShowMessages("\t\te.g : !tsc script { printf(\"RDTSC/P instruction called at: %%llx\\n\", @rip); }\n");
    ShowMessages("\t\te.g : !tsc asm code { nop; nop; nop }\n");
}

/**
 * @brief handler of !tsc command
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandTsc(vector<CommandToken> CommandTokens, string Command)
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
            TSC_INSTRUCTION_EXECUTION,
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

        CommandTscHelp();

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
