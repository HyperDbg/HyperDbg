/**
 * @file e.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief e* command
 * @details
 * @version 0.1
 * @date 2020-07-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN                  g_IsSerialConnectedToRemoteDebuggee;
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;

/**
 * @brief help of !e* and e* commands
 *
 * @return VOID
 */
VOID
CommandEditMemoryHelp()
{
    ShowMessages("eb !eb ed !ed eq !eq : edits the memory at specific address \n");
    ShowMessages("eb  Byte and ASCII characters\n");
    ShowMessages("ed  Double-word values (4 bytes)\n");
    ShowMessages("eq  Quad-word values (8 bytes). \n");

    ShowMessages("\n If you want to edit physical (address) memory then add '!' "
                 "at the start of the command\n");

    ShowMessages("syntax : \teb [Address (hex)] [Contents (hex)] [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \ted [Address (hex)] [Contents (hex)] [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \teq [Address (hex)] [Contents (hex)] [pid ProcessId (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : eb fffff8077356f010 90 \n");
    ShowMessages("\t\te.g : eb nt!Kd_DEFAULT_Mask ff ff ff ff \n");
    ShowMessages("\t\te.g : eb nt!Kd_DEFAULT_Mask+10+@rcx ff ff ff ff \n");
    ShowMessages("\t\te.g : eb fffff8077356f010 90 90 90 90 \n");
    ShowMessages("\t\te.g : !eq 100000 9090909090909090\n");
    ShowMessages("\t\te.g : !eq nt!ExAllocatePoolWithTag+55 9090909090909090\n");
    ShowMessages("\t\te.g : !eq 100000 9090909090909090 9090909090909090 "
                 "9090909090909090 9090909090909090 9090909090909090\n");
}

/**
 * @brief Perform writing the memory content
 *
 * @param AddressToEdit
 * @param MemoryType
 * @param ByteSize
 * @param Pid
 * @param CountOf64Chunks
 * @param BufferToEdit
 *
 * @return BOOLEAN
 */
BOOLEAN
WriteMemoryContent(UINT64                         AddressToEdit,
                   DEBUGGER_EDIT_MEMORY_TYPE      MemoryType,
                   DEBUGGER_EDIT_MEMORY_BYTE_SIZE ByteSize,
                   UINT32                         Pid,
                   UINT32                         CountOf64Chunks,
                   UINT64 *                       BufferToEdit)
{
    BOOL                 Status;
    BOOLEAN              StatusReturn = FALSE;
    UINT64 *             FinalBuffer;
    DEBUGGER_EDIT_MEMORY EditMemoryRequest = {0};
    UINT32               FinalSize         = 0;

    //
    // Check if driver is loaded if it's in VMI mode
    //
    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);
    }

    //
    // Fill the structure
    //
    EditMemoryRequest.ProcessId       = Pid;
    EditMemoryRequest.Address         = AddressToEdit;
    EditMemoryRequest.CountOf64Chunks = CountOf64Chunks;
    EditMemoryRequest.MemoryType      = MemoryType;
    EditMemoryRequest.ByteSize        = ByteSize;

    //
    // Now it's time to put everything together in one structure
    //
    FinalSize = (CountOf64Chunks * sizeof(UINT64)) + SIZEOF_DEBUGGER_EDIT_MEMORY;

    //
    // Set the size
    //
    EditMemoryRequest.FinalStructureSize = FinalSize;

    //
    // Allocate structure + buffer
    //
    FinalBuffer = (UINT64 *)malloc(FinalSize);

    if (!FinalBuffer)
    {
        ShowMessages("unable to allocate memory\n\n");
        return FALSE;
    }

    //
    // Zero the buffer
    //
    ZeroMemory(FinalBuffer, FinalSize);

    //
    // Copy the structure on top of the allocated buffer
    //
    memcpy(FinalBuffer, &EditMemoryRequest, SIZEOF_DEBUGGER_EDIT_MEMORY);

    //
    // Copy the values to the buffer
    //
    memcpy((UINT64 *)((UINT64)FinalBuffer + SIZEOF_DEBUGGER_EDIT_MEMORY), BufferToEdit, (CountOf64Chunks * sizeof(UINT64)));

    //
    // send the request
    //
    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        if (!KdSendEditMemoryPacketToDebuggee((DEBUGGER_EDIT_MEMORY *)FinalBuffer, FinalSize))
        {
            free(FinalBuffer);
            return FALSE;
        }
    }
    else
    {
        Status = DeviceIoControl(
            g_DeviceHandle,              // Handle to device
            IOCTL_DEBUGGER_EDIT_MEMORY,  // IO Control Code (IOCTL)
            FinalBuffer,                 // Input Buffer to driver.
            FinalSize,                   // Input buffer length
            &EditMemoryRequest,          // Output Buffer from driver.
            SIZEOF_DEBUGGER_EDIT_MEMORY, // Length of output buffer in bytes.
            NULL,                        // Bytes placed in buffer.
            NULL                         // synchronous call
        );

        if (!Status)
        {
            ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
            free(FinalBuffer);
            return FALSE;
        }
    }

    //
    // Check the result
    //
    if (EditMemoryRequest.Result == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
    {
        //
        // Was successful, nothing to do
        //
        free(FinalBuffer);
        return TRUE;
    }
    else
    {
        ShowErrorMessage(EditMemoryRequest.Result);
        free(FinalBuffer);
        return FALSE;
    }
}

/**
 * @brief API function for writing the memory content
 *
 * @param AddressToEdit
 * @param MemoryType
 * @param ProcessId
 * @param SourceAddress
 * @param NumberOfBytes
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperDbgWriteMemory(PVOID                     DestinationAddress,
                    DEBUGGER_EDIT_MEMORY_TYPE MemoryType,
                    UINT32                    ProcessId,
                    PVOID                     SourceAddress,
                    UINT32                    NumberOfBytes)
{
    UINT32                         RequiredBytes = 0;
    DEBUGGER_EDIT_MEMORY_BYTE_SIZE ByteSize;
    UINT64 *                       TargetBuffer;
    UINT32                         FinalSize    = 0;
    BOOLEAN                        Result       = FALSE;
    BYTE *                         BufferToEdit = (BYTE *)SourceAddress;

    //
    // Set the byte size to byte granularity
    //
    ByteSize = EDIT_BYTE;

    //
    // Calculate the count of 64 chunks
    //
    RequiredBytes = NumberOfBytes * sizeof(UINT64);

    //
    // Allocate structure + buffer
    //
    TargetBuffer = (UINT64 *)malloc(RequiredBytes);

    if (!TargetBuffer)
    {
        return FALSE;
    }

    //
    // Zero the buffer
    //
    ZeroMemory(TargetBuffer, FinalSize);

    //
    // Copy requested memory in 64bit chunks
    //
    for (size_t i = 0; i < NumberOfBytes; i++)
    {
        TargetBuffer[i] = BufferToEdit[i];
    }

    //
    // Perform the write operation
    //
    Result = WriteMemoryContent((UINT64)DestinationAddress,
                                MemoryType,
                                ByteSize,
                                ProcessId,
                                NumberOfBytes,
                                TargetBuffer);

    //
    // Free the malloc buffer
    //
    free(TargetBuffer);

    return Result;
}

/**
 * @brief !e* and e* commands handler
 *
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandEditMemory(vector<string> SplitCommand, string Command)
{
    UINT64                         Address;
    UINT64 *                       FinalBuffer;
    vector<UINT64>                 ValuesToEdit;
    DEBUGGER_EDIT_MEMORY_TYPE      MemoryType;
    DEBUGGER_EDIT_MEMORY_BYTE_SIZE ByteSize;
    BOOL                           SetAddress    = FALSE;
    BOOL                           SetValue      = FALSE;
    BOOL                           SetProcId     = FALSE;
    BOOL                           NextIsProcId  = FALSE;
    UINT64                         Value         = 0;
    UINT32                         ProcId        = 0;
    UINT32                         CountOfValues = 0;
    UINT32                         FinalSize     = 0;
    vector<string>                 SplitCommandCaseSensitive {Split(Command, ' ')};
    UINT32                         IndexInCommandCaseSensitive = 0;
    BOOLEAN                        IsFirstCommand              = TRUE;

    //
    // By default if the user-debugger is active, we use these commands
    // on the memory layout of the debuggee process
    //
    if (g_ActiveProcessDebuggingState.IsActive)
    {
        ProcId = g_ActiveProcessDebuggingState.ProcessId;
    }

    if (SplitCommand.size() <= 2)
    {
        ShowMessages("incorrect use of the 'e*'\n\n");
        CommandEditMemoryHelp();
        return;
    }

    for (auto Section : SplitCommand)
    {
        IndexInCommandCaseSensitive++;

        if (IsFirstCommand)
        {
            if (!Section.compare("!eb"))
            {
                MemoryType = EDIT_PHYSICAL_MEMORY;
                ByteSize   = EDIT_BYTE;
            }
            else if (!Section.compare("!ed"))
            {
                MemoryType = EDIT_PHYSICAL_MEMORY;
                ByteSize   = EDIT_DWORD;
            }
            else if (!Section.compare("!eq"))
            {
                MemoryType = EDIT_PHYSICAL_MEMORY;
                ByteSize   = EDIT_QWORD;
            }
            else if (!Section.compare("eb"))
            {
                MemoryType = EDIT_VIRTUAL_MEMORY;
                ByteSize   = EDIT_BYTE;
            }
            else if (!Section.compare("ed"))
            {
                MemoryType = EDIT_VIRTUAL_MEMORY;
                ByteSize   = EDIT_DWORD;
            }
            else if (!Section.compare("eq"))
            {
                MemoryType = EDIT_VIRTUAL_MEMORY;
                ByteSize   = EDIT_QWORD;
            }
            else
            {
                //
                // What's this? :(
                //
                ShowMessages("unknown error happened !\n\n");
                CommandEditMemoryHelp();
                return;
            }

            IsFirstCommand = FALSE;

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
                ShowMessages("please specify a correct hex process id\n\n");
                CommandEditMemoryHelp();
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

        //
        // Check if it's a process id or not
        //
        if (!SetProcId && !Section.compare("pid"))
        {
            NextIsProcId = TRUE;
            continue;
        }

        if (!SetAddress)
        {
            if (!SymbolConvertNameOrExprToAddress(SplitCommandCaseSensitive.at(IndexInCommandCaseSensitive - 1),
                                                  &Address))
            {
                ShowMessages("err, couldn't resolve error at '%s'\n\n",
                             SplitCommandCaseSensitive.at(IndexInCommandCaseSensitive - 1).c_str());
                CommandEditMemoryHelp();
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
            else if (Section.rfind('x', 0) == 0 || Section.rfind('X', 0) == 0)
            {
                Section = Section.erase(0, 1);
            }
            Section.erase(remove(Section.begin(), Section.end(), '`'), Section.end());

            //
            // Check if the value is valid based on byte counts
            //
            if (ByteSize == EDIT_BYTE && Section.size() >= 3)
            {
                ShowMessages("please specify a byte (hex) value for 'eb' or '!eb'\n\n");
                return;
            }
            if (ByteSize == EDIT_DWORD && Section.size() >= 9)
            {
                ShowMessages(
                    "please specify a dword (hex) value for 'ed' or '!ed'\n\n");
                return;
            }
            if (ByteSize == EDIT_QWORD && Section.size() >= 17)
            {
                ShowMessages(
                    "please specify a qword (hex) value for 'eq' or '!eq'\n\n");
                return;
            }

            //
            // Qword is checked by the following function, no need to double
            // check it above.
            //

            if (!ConvertStringToUInt64(Section, &Value))
            {
                ShowMessages("please specify a correct hex value to change the memory "
                             "content\n\n");
                CommandEditMemoryHelp();
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
                    // At least on value is there
                    //
                    SetValue = TRUE;
                }
                continue;
            }
        }
    }

    //
    // Check to prevent using process id in e* commands
    //
    if (g_IsSerialConnectedToRemoteDebuggee && ProcId != 0)
    {
        ShowMessages(ASSERT_MESSAGE_CANNOT_SPECIFY_PID);
        return;
    }

    if (ProcId == 0)
    {
        ProcId = GetCurrentProcessId();
    }

    //
    // Check if address and value are set or not
    //
    if (!SetAddress)
    {
        ShowMessages("please specify a correct hex address\n\n");
        CommandEditMemoryHelp();
        return;
    }
    if (!SetValue)
    {
        ShowMessages(
            "please specify a correct hex value as the content to edit\n\n");
        CommandEditMemoryHelp();
        return;
    }
    if (NextIsProcId)
    {
        ShowMessages("please specify a correct hex value as the process id\n\n");
        CommandEditMemoryHelp();
        return;
    }

    //
    // Make the chunks for editing
    //
    FinalSize = (CountOfValues * sizeof(UINT64));

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
    // Put the values in 64 bit structures
    //
    std::copy(ValuesToEdit.begin(), ValuesToEdit.end(), FinalBuffer);

    //
    // Perform the write operation
    //
    WriteMemoryContent(Address,
                       MemoryType,
                       ByteSize,
                       ProcId,
                       CountOfValues,
                       FinalBuffer);

    //
    // Free the malloc buffer
    //
    free(FinalBuffer);
}
