/**
 * @file cpuid.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !cpuid commands
 * @details
 * @version 0.1
 * @date 2020-05-30
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of !cpuid command
 *
 * @return VOID
 */
VOID
CommandCpuidHelp()
{
    ShowMessages("!cpuid : Monitors execution of a special cpuid index or all "
                 "cpuids instructions.\n\n");
    ShowMessages("syntax : \t!cpuid core [core index "
                 "(hex value)] pid [process id (hex value)] condition {[assembly "
                 "in hex]} code {[assembly in hex]} buffer [pre-require buffer - "
                 "(hex value)] \n");

    ShowMessages("\t\te.g : !cpuid\n");
    ShowMessages("\t\te.g : !cpuid pid 400\n");
    ShowMessages("\t\te.g : !cpuid core 2 pid 400\n");
}

/**
 * @brief !cpuid command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandCpuid(vector<string> SplittedCommand, string Command)
{
    PDEBUGGER_GENERAL_EVENT_DETAIL Event                 = NULL;
    PDEBUGGER_GENERAL_ACTION       ActionBreakToDebugger = NULL;
    PDEBUGGER_GENERAL_ACTION       ActionCustomCode      = NULL;
    PDEBUGGER_GENERAL_ACTION       ActionScript          = NULL;
    UINT32                         EventLength;
    UINT32                         ActionBreakToDebuggerLength = 0;
    UINT32                         ActionCustomCodeLength      = 0;
    UINT32                         ActionScriptLength          = 0;

    //
    // Interpret and fill the general event and action fields
    //
    //
    if (!InterpretGeneralEventAndActionsFields(
            &SplittedCommand,
            CPUID_INSTRUCTION_EXECUTION,
            &Event,
            &EventLength,
            &ActionBreakToDebugger,
            &ActionBreakToDebuggerLength,
            &ActionCustomCode,
            &ActionCustomCodeLength,
            &ActionScript,
            &ActionScriptLength))
    {
        CommandCpuidHelp();
        return;
    }

    //
    // Check for size
    //
    if (SplittedCommand.size() > 1)
    {
        ShowMessages("incorrect use of '!cpuid'\n");
        CommandCpuidHelp();
        return;
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
