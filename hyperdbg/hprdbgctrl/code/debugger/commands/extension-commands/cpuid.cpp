/**
 * @file cpuid.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
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
 * @brief help of the !cpuid command
 *
 * @return VOID
 */
VOID
CommandCpuidHelp()
{
    ShowMessages("!cpuid : monitors execution of a special cpuid index or all "
                 "cpuids instructions.\n\n");

    ShowMessages("syntax : \t!cpuid [Eax (hex)] [pid ProcessId (hex)] [core CoreId (hex)] "
                 "[imm IsImmediate (yesno)] [sc EnableShortCircuiting (onoff)] [stage CallingStage (prepostall)] "
                 "[buffer PreAllocatedBuffer (hex)] [script { Script (string) }] [condition { Condition (hex) }] "
                 "[code { Code (hex) }] [output {OutputName (string)}]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !cpuid\n");
    ShowMessages("\t\te.g : !cpuid 1\n");
    ShowMessages("\t\te.g : !cpuid pid 400\n");
    ShowMessages("\t\te.g : !cpuid core 2 pid 400\n");
}

/**
 * @brief !cpuid command handler
 *
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandCpuid(vector<string> SplitCommand, string Command)
{
    PDEBUGGER_GENERAL_EVENT_DETAIL     Event                 = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionBreakToDebugger = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionCustomCode      = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionScript          = NULL;
    BOOLEAN                            GetEax                = FALSE;
    UINT32                             EventLength;
    UINT64                             SpecialTarget               = 0;
    UINT32                             ActionBreakToDebuggerLength = 0;
    UINT32                             ActionCustomCodeLength      = 0;
    UINT32                             ActionScriptLength          = 0;
    vector<string>                     SplitCommandCaseSensitive {Split(Command, ' ')};
    DEBUGGER_EVENT_PARSING_ERROR_CAUSE EventParsingErrorCause;

    //
    // Interpret and fill the general event and action fields
    //
    //
    if (!InterpretGeneralEventAndActionsFields(
            &SplitCommand,
            &SplitCommandCaseSensitive,
            CPUID_INSTRUCTION_EXECUTION,
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
    // Interpret command specific details (if any), of CPUID EAX index
    //
    for (auto Section : SplitCommand)
    {
        if (!Section.compare("!cpuid"))
        {
            continue;
        }
        else if (!GetEax)
        {
            //
            // It's probably an msr
            //
            if (!ConvertStringToUInt64(Section, &SpecialTarget))
            {
                //
                // Unknown parameter
                //
                ShowMessages("unknown parameter '%s'\n\n", Section.c_str());
                CommandCpuidHelp();

                FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
                return;
            }
            else
            {
                //
                // An special EAX is set
                //
                GetEax = TRUE;
            }
        }
        else
        {
            //
            // Unknown parameter
            //
            ShowMessages("unknown parameter '%s'\n\n", Section.c_str());
            CommandCpuidHelp();

            FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
            return;
        }
    }

    //
    // Set the target EAX (if not specific then it means all msrs)
    //
    Event->Options.OptionalParam1 = GetEax;

    if (GetEax)
    {
        Event->Options.OptionalParam2 = SpecialTarget;
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
