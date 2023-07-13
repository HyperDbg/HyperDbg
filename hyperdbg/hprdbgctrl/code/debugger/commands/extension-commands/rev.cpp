/**
 * @file rev.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !rev command
 * @details
 * @version 0.2
 * @date 2023-03-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of the !rev command
 *
 * @return VOID
 */
VOID
CommandRevHelp()
{
    ShowMessages("!rev : uses the reversing machine module in order to reconstruct the programmer/memory assumptions.\n\n");

    ShowMessages("syntax : \t!rev [config] [Path (string)]\n");
    ShowMessages("\n");
    ShowMessages("\t\te.g : !rev pattern c:\\users\\sina\\reverse eng\\config.json\n");
}

/**
 * @brief !rev command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandRev(vector<string> SplittedCommand, string Command)
{
    REVERSING_MACHINE_RECONSTRUCT_MEMORY_REQUEST RevRequest = {0};

    BOOLEAN IgnoreFirstCommand = TRUE;

    UINT64  TargetPid = 0;
    BOOLEAN NextIsPid = FALSE;

    UINT32  TargetSize = 0;
    BOOLEAN NextIsSize = FALSE;

    UINT64  TargetAddress = 0;
    BOOLEAN NextIsAddress = FALSE;

    REVERSING_MACHINE_RECONSTRUCT_MEMORY_MODE Mode = REVERSING_MACHINE_RECONSTRUCT_MEMORY_MODE_UNKNOWN;
    REVERSING_MACHINE_RECONSTRUCT_MEMORY_TYPE Type = REVERSING_MACHINE_RECONSTRUCT_MEMORY_TYPE_UNKNOWN;
    REVERSING_MACHINE_RECONSTRUCT_MEMORY_FORM Form = REVERSING_MACHINE_RECONSTRUCT_MEMORY_FORM_UNKNOWN;

    //
    // Send the request to the hypervisor (kernel)
    //
    RevRequestService(&RevRequest);
}
