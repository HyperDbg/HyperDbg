/**
 * @file s.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief s* command
 * @details
 * @version 0.1
 * @date 2020-07-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of !s* s* commands
 *
 * @return VOID
 */
VOID
CommandSearchMemoryHelp()
{
    ShowMessages("sb !sb sd !sd sq !sq : searches a contiguous memory for a "
                 "special byte pattern\n");
    ShowMessages("s[b]  Byte and ASCII characters\n");
    ShowMessages("s[d]  Double-word values (4 bytes)\n");
    ShowMessages("s[q]  Quad-word values (8 bytes). \n");
    ShowMessages(
        "\n If you want to search in physical (address) memory then add '!' "
        "at the start of the command\n");

    ShowMessages("syntax : \t[!]s[b|d|q] [address from] l [length (hex value)] "
                 "[byte pattern (hex)] pid [process id (hex)]\n");

    ShowMessages("\t\te.g : sb fffff8077356f010 90 85 95 l ffff \n");
    ShowMessages("\t\te.g : sd fffff8077356f010 90423580 l ffff pid 1c0 \n");
    ShowMessages("\t\te.g : !sq 100000 9090909090909090 l ffff\n");
    ShowMessages("\t\te.g : !sq 100000 9090909090909090 9090909090909090 "
                 "9090909090909090 l ffffff\n");
}

/**
 * @brief !s* s* commands handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandSearchMemory(vector<string> SplittedCommand, string Command)
{
    BOOL                   Status;
    BOOL                   SetAddress          = FALSE;
    BOOL                   SetValue            = FALSE;
    BOOL                   SetProcId           = FALSE;
    BOOL                   NextIsProcId        = FALSE;
    BOOL                   SetLength           = FALSE;
    BOOL                   NextIsLength        = FALSE;
    DEBUGGER_SEARCH_MEMORY SearchMemoryRequest = {0};
    PUINT64                ResultsBuffer;
    UINT64                 CurrentValue;
    UINT64                 Address;
    UINT64                 Value         = 0;
    UINT64                 Length        = 0;
    UINT32                 ProcId        = GetCurrentProcessId();
    UINT32                 CountOfValues = 0;
    UINT32                 FinalSize     = 0;
    UINT64 *               FinalBuffer;
    vector<UINT64>         ValuesToEdit;

    if (SplittedCommand.size() <= 4)
    {
        ShowMessages("incorrect use of 's*'\n\n");
        CommandSearchMemoryHelp();
        return;
    }

    for (auto Section : SplittedCommand)
    {
        if (!Section.compare(SplittedCommand.at(0)))
        {
            if (!Section.compare("!sb"))
            {
                SearchMemoryRequest.MemoryType = SEARCH_PHYSICAL_MEMORY;
                SearchMemoryRequest.ByteSize   = SEARCH_BYTE;
            }
            else if (!Section.compare("!sd"))
            {
                SearchMemoryRequest.MemoryType = SEARCH_PHYSICAL_MEMORY;
                SearchMemoryRequest.ByteSize   = SEARCH_DWORD;
            }
            else if (!Section.compare("!sq"))
            {
                SearchMemoryRequest.MemoryType = SEARCH_PHYSICAL_MEMORY;
                SearchMemoryRequest.ByteSize   = SEARCH_QWORD;
            }
            else if (!Section.compare("sb"))
            {
                SearchMemoryRequest.MemoryType = SEARCH_VIRTUAL_MEMORY;
                SearchMemoryRequest.ByteSize   = SEARCH_BYTE;
            }
            else if (!Section.compare("sd"))
            {
                SearchMemoryRequest.MemoryType = SEARCH_VIRTUAL_MEMORY;
                SearchMemoryRequest.ByteSize   = SEARCH_DWORD;
            }
            else if (!Section.compare("sq"))
            {
                SearchMemoryRequest.MemoryType = SEARCH_VIRTUAL_MEMORY;
                SearchMemoryRequest.ByteSize   = SEARCH_QWORD;
            }
            else
            {
                //
                // What's this? :(
                //
                ShowMessages("unknown error happened !\n\n");
                CommandSearchMemoryHelp();
                return;
            }

            continue;
        }

        if (NextIsProcId)
        {
            //
            // It's a process id
            //
            NextIsProcId = FALSE;

            if (!ConvertStringToUInt32(Section, &ProcId))
            {
                ShowMessages("please specify a correct hex prcoess id\n\n");
                CommandSearchMemoryHelp();
                return;
            }
            else
            {
                //
                // Means that the proc id is set, next we should read value
                //
                continue;
            }
        }

        if (NextIsLength)
        {
            //
            // It's a length
            //
            NextIsLength = FALSE;

            if (!ConvertStringToUInt64(Section, &Length))
            {
                ShowMessages("please specify a correct hex length\n\n");
                CommandSearchMemoryHelp();
                return;
            }
            else
            {
                //
                // Means that the proc id is set, next we should read value
                //
                SetLength = TRUE;
                continue;
            }
        }

        //
        // Check if it's a process id or not
        //
        if (!SetProcId && !Section.compare("pid"))
        {
            NextIsProcId = TRUE;
            continue;
        }

        //
        // Check if it's a length or not
        //
        if (!SetLength && !Section.compare("l"))
        {
            NextIsLength = TRUE;
            continue;
        }

        if (!SetAddress)
        {
            if (!ConvertStringToUInt64(Section, &Address))
            {
                ShowMessages("please specify a correct hex address to search\n\n");
                CommandSearchMemoryHelp();
                return;
            }
            else
            {
                //
                // Means that the address is set, next we should read value
                //
                SetAddress = TRUE;
                continue;
            }
        }

        if (SetAddress)
        {
            //
            // Remove the hex notations
            //
            if (Section.rfind("0x", 0) == 0 || Section.rfind("0X", 0) == 0 ||
                Section.rfind("\\x", 0) == 0 || Section.rfind("\\X", 0) == 0)
            {
                Section = Section.erase(0, 2);
            }
            else if (Section.rfind("x", 0) == 0 || Section.rfind("X", 0) == 0)
            {
                Section = Section.erase(0, 1);
            }
            Section.erase(remove(Section.begin(), Section.end(), '`'), Section.end());

            //
            // Check if the value is valid based on byte counts
            //
            if (SearchMemoryRequest.ByteSize == SEARCH_BYTE && Section.size() >= 3)
            {
                ShowMessages("please specify a byte (hex) value for 'sb' or '!sb'\n\n");
                return;
            }
            if (SearchMemoryRequest.ByteSize == SEARCH_DWORD && Section.size() >= 9)
            {
                ShowMessages(
                    "please specify a dword (hex) value for 'sd' or '!sd'\n\n");
                return;
            }
            if (SearchMemoryRequest.ByteSize == SEARCH_QWORD &&
                Section.size() >= 17)
            {
                ShowMessages(
                    "please specify a qword (hex) value for 'sq' or '!sq'\n\n");
                return;
            }

            //
            // Qword is checked by the following function, no need to double
            // check it above.
            //
            if (!ConvertStringToUInt64(Section, &Value))
            {
                ShowMessages("please specify a correct hex value to search in the "
                             "memory content\n\n");
                CommandSearchMemoryHelp();
                return;
            }
            else
            {
                //
                // Add it to the list
                //
                ValuesToEdit.push_back(Value);

                //
                // Keep track of values to modify
                //
                CountOfValues++;

                if (!SetValue)
                {
                    //
                    // At least on walue is there
                    //
                    SetValue = TRUE;
                }
                continue;
            }
        }
    }

    //
    // Fill the structure
    //
    SearchMemoryRequest.ProcessId       = ProcId;
    SearchMemoryRequest.Address         = Address;
    SearchMemoryRequest.CountOf64Chunks = CountOfValues;

    //
    // Check if address and value are set or not
    //
    if (!SetAddress)
    {
        ShowMessages("please specify a correct hex address\n\n");
        CommandSearchMemoryHelp();
        return;
    }
    if (!SetValue)
    {
        ShowMessages(
            "please specify a correct hex value as the content to search\n\n");
        CommandSearchMemoryHelp();
        return;
    }
    if (!SetLength)
    {
        ShowMessages("please specify a correct hex value as the length\n\n");
        CommandSearchMemoryHelp();
        return;
    }
    if (NextIsProcId)
    {
        ShowMessages("please specify a correct hex value as the process id\n\n");
        CommandSearchMemoryHelp();
        return;
    }
    if (NextIsLength)
    {
        ShowMessages("please specify a correct hex length\n\n");
        CommandSearchMemoryHelp();
        return;
    }

    if (!g_DeviceHandle)
    {
        ShowMessages("Handle not found, probably the driver is not loaded. Did you "
                     "use 'load' command?\n");
        return;
    }

    //
    // Now it's time to put everything together in one structure
    //
    FinalSize = (CountOfValues * sizeof(UINT64)) + SIZEOF_DEBUGGER_SEARCH_MEMORY;

    //
    // Set the size
    //
    SearchMemoryRequest.FinalStructureSize = FinalSize;

    //
    // Set the length
    //
    SearchMemoryRequest.Length = Length;

    //
    // Allocate structure + buffer
    //
    FinalBuffer = (UINT64 *)malloc(FinalSize);

    if (!FinalBuffer)
    {
        ShowMessages("unable to allocate memory\n\n");
        return;
    }

    //
    // Zero the buffer
    //
    ZeroMemory(FinalBuffer, FinalSize);

    //
    // Copy the structure on top of the allocated buffer
    //
    memcpy(FinalBuffer, &SearchMemoryRequest, SIZEOF_DEBUGGER_SEARCH_MEMORY);

    //
    // Put the values in 64 bit structures
    //
    std::copy(ValuesToEdit.begin(), ValuesToEdit.end(), (UINT64 *)((UINT64)FinalBuffer + SIZEOF_DEBUGGER_SEARCH_MEMORY));

    //
    // Allocate a buffer to store the results
    //
    ResultsBuffer = (PUINT64)malloc(MaximumSearchResults * sizeof(UINT64));

    //
    // Also it's better to Zero the memory; however it's not necessary
    // as we zero the buffer in the search routines
    //
    ZeroMemory(ResultsBuffer, MaximumSearchResults * sizeof(UINT64));

    //
    // Fire the IOCTL
    //
    Status =
        DeviceIoControl(g_DeviceHandle,               // Handle to device
                        IOCTL_DEBUGGER_SEARCH_MEMORY, // IO Control code
                        FinalBuffer,                  // Input Buffer to driver.
                        FinalSize,                    // Input buffer length
                        ResultsBuffer,                // Output Buffer from driver.
                        MaximumSearchResults *
                            sizeof(UINT64), // Length of output buffer in bytes.
                        NULL,               // Bytes placed in buffer.
                        NULL                // synchronous call
        );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        free(FinalBuffer);
        return;
    }

    //
    // Show the results (if any)
    //
    for (size_t i = 0; i < MaximumSearchResults; i++)
    {
        CurrentValue = ResultsBuffer[i];

        if (CurrentValue == NULL)
        {
            //
            // We ended up the buffer, nothing else to show,
            // just check whether we found anything or not
            //
            if (i == 0)
            {
                ShowMessages("Not found.\n");
            }
            break;
        }
        ShowMessages("%llx\n", CurrentValue);
    }

    //
    // Free the buffers
    //
    free(FinalBuffer);
    free(ResultsBuffer);
}
