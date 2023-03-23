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
 * @brief help of !rev command
 *
 * @return VOID
 */
VOID
CommandRevHelp()
{
    ShowMessages("!rev : uses the reversing machine module in order to reconstruct the memory assumptions.\n\n");

    ShowMessages("syntax : \t!rev [reconstruct] [kernel] [address Address (hex)] [size Size (hex)]\n");
    ShowMessages("syntax : \t!rev [reconstruct] [kernel] [overall]\n");
    ShowMessages("syntax : \t!rev [reconstruct] [user] [overall] [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \t!rev [reconstruct] [user] [overall] [path Path (string)]\n");
    ShowMessages(
        "syntax : \t!rev [reconstruct] [user] [pid ProcessId (hex)] [address Address (hex)] [size Size (hex)]\n");
    ShowMessages("syntax : \t!rev [pattern] [path Path (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !rev reconstruct kernel overall\n");
    ShowMessages("\t\te.g : !rev reconstruct kernel address fffff8077356f010 size 1000\n");
    ShowMessages("\t\te.g : !rev reconstruct kernel address fffff8077356f010\n");
    ShowMessages("\t\te.g : !rev reconstruct user overall pid 1c0\n");
    ShowMessages("\t\te.g : !rev reconstruct user overall path c:\\users\\sina\\reverse eng\\my_file.exe\n");
    ShowMessages("\t\te.g : !rev reconstruct user pid 1c0 address 0x7ff0001000\n");
    ShowMessages("\t\te.g : !rev reconstruct user pid 1c0 address 0x7ff0001000 size 1000\n");
    ShowMessages("\t\te.g : !rev pattern path c:\\users\\sina\\reverse eng\\my_file.exe\n");
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
    // Check the minimum argument
    //
    if (SplittedCommand.size() <= 3)
    {
        ShowMessages("incorrect use of '!rev'\n\n");
        CommandRevHelp();
        return;
    }

    for (auto item : SplittedCommand)
    {
        //
        // Find out whether the user enters pid or not
        //
        if (IgnoreFirstCommand)
        {
            IgnoreFirstCommand = FALSE;
        }
        else if (NextIsPid)
        {
            NextIsPid = FALSE;

            if (!ConvertStringToUInt64(item, &TargetPid))
            {
                ShowMessages("please specify a correct hex value for process id\n\n");
                CommandRevHelp();
                return;
            }
        }
        else if (NextIsAddress)
        {
            NextIsAddress = FALSE;

            if (!ConvertStringToUInt64(item, &TargetAddress))
            {
                ShowMessages("please specify a correct hex value for address\n\n");
                CommandRevHelp();
                return;
            }
        }
        else if (NextIsSize)
        {
            NextIsSize = FALSE;

            if (!ConvertStringToUInt32(item, &TargetSize))
            {
                ShowMessages("please specify a correct hex value for size\n\n");
                CommandRevHelp();
                return;
            }
        }
        else if (!item.compare("pid"))
        {
            //
            // next item is a pid for the process
            //
            NextIsPid = TRUE;
        }
        else if (!item.compare("address"))
        {
            //
            // next item is a address of memory
            //
            Form          = REVERSING_MACHINE_RECONSTRUCT_MEMORY_FORM_ADDRESS_BASED;
            NextIsAddress = TRUE;
        }
        else if (!item.compare("size"))
        {
            //
            // next item is a memory size
            //
            NextIsSize = TRUE;
        }
        else if (!item.compare("user") && Mode == REVERSING_MACHINE_RECONSTRUCT_MEMORY_MODE_UNKNOWN)
        {
            Mode = REVERSING_MACHINE_RECONSTRUCT_MEMORY_MODE_USER_MODE;
        }
        else if (!item.compare("kernel") && Mode == REVERSING_MACHINE_RECONSTRUCT_MEMORY_MODE_UNKNOWN)
        {
            Mode = REVERSING_MACHINE_RECONSTRUCT_MEMORY_MODE_KERNEL_MODE;
        }
        else if ((!item.compare("reconstruct") || !item.compare("reconst")) && Type == REVERSING_MACHINE_RECONSTRUCT_MEMORY_TYPE_UNKNOWN)
        {
            Type = REVERSING_MACHINE_RECONSTRUCT_MEMORY_TYPE_RECONSTRUCT;
        }
        else if (!item.compare("pattern") && Type == REVERSING_MACHINE_RECONSTRUCT_MEMORY_TYPE_UNKNOWN)
        {
            Type = REVERSING_MACHINE_RECONSTRUCT_MEMORY_TYPE_PATTERN;
        }
        else if (!item.compare("overall") && Form == REVERSING_MACHINE_RECONSTRUCT_MEMORY_FORM_UNKNOWN)
        {
            Form = REVERSING_MACHINE_RECONSTRUCT_MEMORY_FORM_OVERALL;
        }
        else
        {
            ShowMessages(
                "err, unknown parameter at '%s'\n\n",
                item.c_str());
            CommandRevHelp();
            return;
        }
    }

    //
    // Check if the pid is empty or not (after "pid")
    //
    if (NextIsPid)
    {
        ShowMessages("please specify a hex value for the process id\n\n");
        CommandRevHelp();
        return;
    }

    //
    // Check if the address is empty or not (after "address")
    //
    if (NextIsAddress)
    {
        ShowMessages("please specify a hex value for the target address\n\n");
        CommandRevHelp();
        return;
    }

    //
    // Check if the size is empty or not (after "size")
    //
    if (NextIsSize)
    {
        ShowMessages("please specify a hex value for the size\n\n");
        CommandRevHelp();
        return;
    }

    //
    // Check if the reconstruction type is specified or not
    //
    if (Type == REVERSING_MACHINE_RECONSTRUCT_MEMORY_TYPE_UNKNOWN)
    {
        ShowMessages("please specify the type of reconstruction (reconstruct or pattern)\n\n");
        CommandRevHelp();
        return;
    }

    //
    // Check if the user specified the execution mode or not
    //
    if (Mode == REVERSING_MACHINE_RECONSTRUCT_MEMORY_MODE_UNKNOWN)
    {
        ShowMessages("please specify the execution mode (user or kernel)\n\n");
        CommandRevHelp();
        return;
    }

    //
    // Check if the process id is empty or not in the case of user-mode execution
    //
    if (TargetPid == 0 && Mode == REVERSING_MACHINE_RECONSTRUCT_MEMORY_MODE_USER_MODE)
    {
        ShowMessages("in the case of user-mode interception, you should "
                     "specify a hex value for process id\n\n");
        CommandRevHelp();
        return;
    }

    //
    // Adjust the request parameters
    //
    RevRequest.Mode           = Mode;
    RevRequest.ProcessId      = TargetPid;
    RevRequest.Size           = TargetSize;
    RevRequest.Type           = Type;
    RevRequest.VirtualAddress = TargetAddress;
    RevRequest.Form           = Form;

    //
    // Send the request to the hypervisor (kernel)
    //
    RevRequestService(&RevRequest);
}
