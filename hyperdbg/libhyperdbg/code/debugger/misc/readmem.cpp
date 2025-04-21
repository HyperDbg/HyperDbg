/**
 * @file readmem.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @author Alee Amini (aleeamini@gmail.com)
 * @brief HyperDbg command for u and d*
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief Read memory and disassembler
 *
 * @param TargetAddress location of where to read the memory
 * @param MemoryType type of memory (phyical or virtual)
 * @param ReadingType read from kernel or vmx-root
 * @param Pid The target process id
 * @param Size size of memory to read
 * @param GetAddressMode check for address mode
 * @param AddressMode Address mode (32 or 64)
 * @param TargetBufferToStore The buffer to store the read memory
 * @param ReturnLength The length of the read memory
 *
 * @return BOOLEAN TRUE if the operation was successful, otherwise FALSE
 */
BOOLEAN
HyperDbgReadMemory(UINT64                              TargetAddress,
                   DEBUGGER_READ_MEMORY_TYPE           MemoryType,
                   DEBUGGER_READ_READING_TYPE          ReadingType,
                   UINT32                              Pid,
                   UINT32                              Size,
                   BOOLEAN                             GetAddressMode,
                   DEBUGGER_READ_MEMORY_ADDRESS_MODE * AddressMode,
                   BYTE *                              TargetBufferToStore,
                   UINT32 *                            ReturnLength)
{
    BOOL                 Status;
    ULONG                ReturnedLength;
    DEBUGGER_READ_MEMORY ReadMem = {0};
    UINT32               SizeOfTargetBuffer;

    //
    // Check if driver is loaded if it's in VMI mode
    //
    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        //   AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);
    }

    //
    // Fill the read memory structure
    //
    ReadMem.Address        = TargetAddress;
    ReadMem.Pid            = Pid;
    ReadMem.Size           = Size;
    ReadMem.MemoryType     = MemoryType;
    ReadMem.ReadingType    = ReadingType;
    ReadMem.GetAddressMode = GetAddressMode;

    //
    // allocate buffer for transferring messages
    //
    SizeOfTargetBuffer                    = sizeof(DEBUGGER_READ_MEMORY) + (Size * sizeof(CHAR));
    DEBUGGER_READ_MEMORY * MemReadRequest = (DEBUGGER_READ_MEMORY *)malloc(SizeOfTargetBuffer);

    //
    // Check if the buffer is allocated successfully
    //
    if (MemReadRequest == NULL)
    {
        return FALSE;
    }

    ZeroMemory(MemReadRequest, SizeOfTargetBuffer);

    //
    // Copy the buffer to send
    //
    memcpy(MemReadRequest, &ReadMem, sizeof(DEBUGGER_READ_MEMORY));

    //
    // Check if this is used for Debugger Mode or VMI mode
    //
    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // It's on Debugger mode
        //
        if (!KdSendReadMemoryPacketToDebuggee(MemReadRequest, SizeOfTargetBuffer))
        {
            std::free(MemReadRequest);
            return FALSE;
        }
    }
    else
    {
        //
        // It's on VMI mode
        //

        Status = DeviceIoControl(g_DeviceHandle,              // Handle to device
                                 IOCTL_DEBUGGER_READ_MEMORY,  // IO Control Code (IOCTL)
                                 MemReadRequest,              // Input Buffer to driver.
                                 SIZEOF_DEBUGGER_READ_MEMORY, // Input buffer length
                                 MemReadRequest,              // Output Buffer from driver.
                                 SizeOfTargetBuffer,          // Length of output buffer in bytes.
                                 &ReturnedLength,             // Bytes placed in buffer.
                                 NULL                         // synchronous call
        );

        if (!Status)
        {
            //  ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
            std::free(MemReadRequest);
            //  return FALSE;
        }
    }

    //
    // Check if reading memory was successful or not
    //
    if (MemReadRequest->KernelStatus != DEBUGGER_OPERATION_WAS_SUCCESSFUL)
    {
        ShowErrorMessage(MemReadRequest->KernelStatus);
        std::free(MemReadRequest);
        return FALSE;
    }
    else
    {
        if (g_IsSerialConnectedToRemoteDebuggee)
        {
            //
            // Change the ReturnedLength as it contains the headers
            //
            *ReturnLength = MemReadRequest->ReturnLength;
        }
        else
        {
            //
            // Change the ReturnedLength as it contains the headers
            //
            ReturnedLength -= SIZEOF_DEBUGGER_READ_MEMORY;
            *ReturnLength = ReturnedLength;
        }

        //
        // Set address mode (if requested)
        //
        if (GetAddressMode)
        {
            *AddressMode = MemReadRequest->AddressMode;
        }

        //
        // Copy the buffer
        //
        memcpy(TargetBufferToStore,
               ((unsigned char *)MemReadRequest) + sizeof(DEBUGGER_READ_MEMORY),
               *ReturnLength);

        //
        // free the buffer
        //
        std::free(MemReadRequest);

        return TRUE;
    }
}

/**
 * @brief Show memory or disassembler
 *
 * @param Style style of show memory (as byte, dwrod, qword)
 * @param Address location of where to read the memory
 * @param MemoryType type of memory (phyical or virtual)
 * @param ReadingType read from kernel or vmx-root
 * @param Pid The target process id
 * @param Size size of memory to read
 * @param DtDetails Options for dt structure show details
 *
 * @return VOID
 */
VOID
HyperDbgShowMemoryOrDisassemble(DEBUGGER_SHOW_MEMORY_STYLE   Style,
                                UINT64                       Address,
                                DEBUGGER_READ_MEMORY_TYPE    MemoryType,
                                DEBUGGER_READ_READING_TYPE   ReadingType,
                                UINT32                       Pid,
                                UINT32                       Size,
                                PDEBUGGER_DT_COMMAND_OPTIONS DtDetails)
{
    UINT32                            ReturnedLength;
    UCHAR *                           Buffer;
    DEBUGGER_READ_MEMORY_ADDRESS_MODE AddressMode;
    BOOLEAN                           CheckForAddressMode = FALSE;
    BOOLEAN                           Status              = FALSE;

    //
    // Check if this is used for disassembler or not
    //
    if (Style == DEBUGGER_SHOW_COMMAND_DISASSEMBLE64 ||
        Style == DEBUGGER_SHOW_COMMAND_DISASSEMBLE32)
    {
        CheckForAddressMode = TRUE;
    }
    else
    {
        CheckForAddressMode = FALSE;
    }

    //
    // Allocate buffer for output
    //
    Buffer = (UCHAR *)malloc(Size);

    //
    // Perform reading memory
    //
    Status = HyperDbgReadMemory(Address,
                                MemoryType,
                                ReadingType,
                                Pid,
                                Size,
                                CheckForAddressMode,
                                &AddressMode,
                                (BYTE *)Buffer,
                                &ReturnedLength);

    //
    // Check if reading memory was successful or not
    //
    if (!Status)
    {
        //
        // Check for extra message for the dump command
        //
        if (Style == DEBUGGER_SHOW_COMMAND_DUMP)
        {
            ShowMessages("HyperDbg attempted to access an invalid target address: 0x%llx\n"
                         "if you are confident that the address is valid, it may be paged out "
                         "or not yet available in the current CR3 page table\n"
                         "you can use the '.pagein' command to load this page table into memory and "
                         "trigger a page fault (#PF), please refer to the documentation for further details\n\n",
                         Address);
        }

        //
        // free the buffer
        //
        std::free(Buffer);
        return;
    }

    switch (Style)
    {
    case DEBUGGER_SHOW_COMMAND_DT:

        //
        // Show the 'dt' command view
        //
        if (Size == ReturnedLength)
        {
            ScriptEngineShowDataBasedOnSymbolTypesWrapper(DtDetails->TypeName,
                                                          Address,
                                                          FALSE,
                                                          Buffer,
                                                          DtDetails->AdditionalParameters);
        }
        else if (ReturnedLength == 0)
        {
            ShowMessages("err, invalid address");
        }
        else
        {
            ShowMessages("err, invalid address or memory is smaller than the structure size");
        }

        break;

    case DEBUGGER_SHOW_COMMAND_DB:

        ShowMemoryCommandDB(
            Buffer,
            Size,
            Address,
            MemoryType,
            ReturnedLength);

        break;

    case DEBUGGER_SHOW_COMMAND_DC:

        ShowMemoryCommandDC(
            Buffer,
            Size,
            Address,
            MemoryType,
            ReturnedLength);

        break;

    case DEBUGGER_SHOW_COMMAND_DD:

        ShowMemoryCommandDD(
            Buffer,
            Size,
            Address,
            MemoryType,
            ReturnedLength);

        break;

    case DEBUGGER_SHOW_COMMAND_DQ:

        ShowMemoryCommandDQ(
            Buffer,
            Size,
            Address,
            MemoryType,
            ReturnedLength);

        break;

    case DEBUGGER_SHOW_COMMAND_DUMP:

        CommandDumpSaveIntoFile(Buffer, Size);

        break;

    case DEBUGGER_SHOW_COMMAND_DISASSEMBLE64:

        //
        // Check if assembly mismatch occurred with the target address
        //
        if (AddressMode == DEBUGGER_READ_ADDRESS_MODE_32_BIT && MemoryType == DEBUGGER_READ_VIRTUAL_ADDRESS)
        {
            ShowMessages("the target address seems to be located in a 32-bit program, if so, "
                         "please consider using the 'u32' instead to utilize the 32-bit disassembler\n");
        }

        //
        // Show diassembles
        //
        if (ReturnedLength != 0)
        {
            UCHAR * Buffer2 = (UCHAR *)"\x48\x3d\x48\x40\x40\x00";
            HyperDbgDisassembler64(
                Buffer,
                Address,
                6,
                0,
                FALSE,
                NULL);
        }
        else
        {
            ShowMessages("err, invalid address\n");
        }

        break;

    case DEBUGGER_SHOW_COMMAND_DISASSEMBLE32:

        //
        // Check if assembly mismatch occurred with the target address
        //
        if (AddressMode == DEBUGGER_READ_ADDRESS_MODE_64_BIT && MemoryType == DEBUGGER_READ_VIRTUAL_ADDRESS)
        {
            ShowMessages("the target address seems to be located in a 64-bit program, if so, "
                         "please consider using the 'u' instead to utilize the 64-bit disassembler\n");
        }

        //
        // Show diassembles
        //
        if (ReturnedLength != 0)
        {
            HyperDbgDisassembler32(
                Buffer,
                Address,
                ReturnedLength,
                0,
                FALSE,
                NULL);
        }
        else
        {
            ShowMessages("err, invalid address\n");
        }

        break;
    }

    //
    // free the buffer
    //
    std::free(Buffer);
}

/**
 * @brief Show memory in bytes (DB)
 *
 * @param OutputBuffer the buffer to show
 * @param Size size of memory to read
 * @param Address location of where to read the memory
 * @param MemoryType type of memory (phyical or virtual)
 * @param Length Length of memory to show
 */
void
ShowMemoryCommandDB(unsigned char * OutputBuffer, UINT32 Size, UINT64 Address, DEBUGGER_READ_MEMORY_TYPE MemoryType, UINT64 Length)
{
    unsigned int Character;

    for (UINT32 i = 0; i < Size; i += 16)
    {
        if (MemoryType == DEBUGGER_READ_PHYSICAL_ADDRESS)
        {
            ShowMessages("#\t");
        }

        //
        // Print address
        //
        ShowMessages("%s  ", SeparateTo64BitValue((UINT64)(Address + i)).c_str());

        //
        // Print the hex code
        //
        for (size_t j = 0; j < 16; j++)
        {
            //
            // check to see if the address is valid or not
            //
            if (i + j >= Length)
            {
                ShowMessages("?? ");
            }
            else
            {
                ShowMessages("%02X ", OutputBuffer[i + j]);
            }
        }

        //
        // Print the character
        //
        ShowMessages(" ");
        for (size_t j = 0; j < 16; j++)
        {
            Character = (OutputBuffer[i + j]);
            if (isprint(Character))
            {
                ShowMessages("%c", Character);
            }
            else
            {
                ShowMessages(".");
            }
        }

        //
        // Go to new line
        //
        ShowMessages("\n");
    }
}

/**
 * @brief Show memory in dword format (DC)
 *
 * @param OutputBuffer the buffer to show
 * @param Size size of memory to read
 * @param Address location of where to read the memory
 * @param MemoryType type of memory (phyical or virtual)
 * @param Length Length of memory to show
 */
void
ShowMemoryCommandDC(unsigned char * OutputBuffer, UINT32 Size, UINT64 Address, DEBUGGER_READ_MEMORY_TYPE MemoryType, UINT64 Length)
{
    unsigned int Character;
    for (UINT32 i = 0; i < Size; i += 16)
    {
        if (MemoryType == DEBUGGER_READ_PHYSICAL_ADDRESS)
        {
            ShowMessages("#\t");
        }

        //
        // Print address
        //
        ShowMessages("%s  ", SeparateTo64BitValue((UINT64)(Address + i)).c_str());

        //
        // Print the hex code
        //
        for (size_t j = 0; j < 16; j += 4)
        {
            //
            // check to see if the address is valid or not
            //
            if (i + j >= Length)
            {
                ShowMessages("???????? ");
            }
            else
            {
                UINT32 OutputBufferVar = *((UINT32 *)&OutputBuffer[i + j]);
                ShowMessages("%08X ", OutputBufferVar);
            }
        }

        //
        // Print the character
        //

        ShowMessages(" ");
        for (size_t j = 0; j < 16; j++)
        {
            Character = (OutputBuffer[i + j]);
            if (isprint(Character))
            {
                ShowMessages("%c", Character);
            }
            else
            {
                ShowMessages(".");
            }
        }

        //
        // Go to new line
        //
        ShowMessages("\n");
    }
}

/**
 * @brief Show memory in dword format (DD)
 *
 * @param OutputBuffer the buffer to show
 * @param Size size of memory to read
 * @param Address location of where to read the memory
 * @param MemoryType type of memory (phyical or virtual)
 * @param Length Length of memory to show
 */
void
ShowMemoryCommandDD(unsigned char * OutputBuffer, UINT32 Size, UINT64 Address, DEBUGGER_READ_MEMORY_TYPE MemoryType, UINT64 Length)
{
    for (UINT32 i = 0; i < Size; i += 16)
    {
        if (MemoryType == DEBUGGER_READ_PHYSICAL_ADDRESS)
        {
            ShowMessages("#\t");
        }

        //
        // Print address
        //
        ShowMessages("%s  ", SeparateTo64BitValue((UINT64)(Address + i)).c_str());

        //
        // Print the hex code
        //
        for (size_t j = 0; j < 16; j += 4)
        {
            //
            // check to see if the address is valid or not
            //
            if (i + j >= Length)
            {
                ShowMessages("???????? ");
            }
            else
            {
                UINT32 OutputBufferVar = *((UINT32 *)&OutputBuffer[i + j]);
                ShowMessages("%08X ", OutputBufferVar);
            }
        }
        //
        // Go to new line
        //
        ShowMessages("\n");
    }
}

/**
 * @brief Show memory in qword format (DQ)
 *
 * @param OutputBuffer the buffer to show
 * @param Size size of memory to read
 * @param Address location of where to read the memory
 * @param MemoryType type of memory (phyical or virtual)
 * @param Length Length of memory to show
 */
void
ShowMemoryCommandDQ(unsigned char * OutputBuffer, UINT32 Size, UINT64 Address, DEBUGGER_READ_MEMORY_TYPE MemoryType, UINT64 Length)
{
    for (UINT32 i = 0; i < Size; i += 16)
    {
        if (MemoryType == DEBUGGER_READ_PHYSICAL_ADDRESS)
        {
            ShowMessages("#\t");
        }

        //
        // Print address
        //
        ShowMessages("%s  ", SeparateTo64BitValue((UINT64)(Address + i)).c_str());

        //
        // Print the hex code
        //
        for (size_t j = 0; j < 16; j += 8)
        {
            //
            // check to see if the address is valid or not
            //
            if (i + j >= Length)
            {
                ShowMessages("???????? ");
            }
            else
            {
                UINT32 OutputBufferVar = *((UINT32 *)&OutputBuffer[i + j + 4]);
                ShowMessages("%08X`", OutputBufferVar);

                OutputBufferVar = *((UINT32 *)&OutputBuffer[i + j]);
                ShowMessages("%08X ", OutputBufferVar);
            }
        }

        //
        // Go to new line
        //
        ShowMessages("\n");
    }
}
