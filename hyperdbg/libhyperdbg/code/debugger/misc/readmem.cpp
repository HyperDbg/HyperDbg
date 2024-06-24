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
HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_MEMORY_STYLE   Style,
                                 UINT64                       Address,
                                 DEBUGGER_READ_MEMORY_TYPE    MemoryType,
                                 DEBUGGER_READ_READING_TYPE   ReadingType,
                                 UINT32                       Pid,
                                 UINT32                       Size,
                                 PDEBUGGER_DT_COMMAND_OPTIONS DtDetails)
{
    BOOL                 Status;
    ULONG                ReturnedLength;
    DEBUGGER_READ_MEMORY ReadMem = {0};
    UINT32               SizeOfTargetBuffer;

    ReadMem.Address     = Address;
    ReadMem.Pid         = Pid;
    ReadMem.Size        = Size;
    ReadMem.MemoryType  = MemoryType;
    ReadMem.ReadingType = ReadingType;
    ReadMem.Style       = Style;
    ReadMem.DtDetails   = DtDetails;

    //
    // Check if this is used for disassembler or not
    //
    if (Style == DEBUGGER_SHOW_COMMAND_DISASSEMBLE64 ||
        Style == DEBUGGER_SHOW_COMMAND_DISASSEMBLE32)
    {
        ReadMem.IsForDisasm = TRUE;
    }
    else
    {
        ReadMem.IsForDisasm = FALSE;
    }

    //
    // send the request
    //
    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        KdSendReadMemoryPacketToDebuggee(&ReadMem);
        return;
    }

    //
    // It's on VMI mode
    //
    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturn);

    //
    // allocate buffer for transferring messages
    //
    SizeOfTargetBuffer           = (Size * sizeof(CHAR)) + sizeof(DEBUGGER_READ_MEMORY);
    unsigned char * OutputBuffer = (unsigned char *)malloc(SizeOfTargetBuffer);

    ZeroMemory(OutputBuffer, SizeOfTargetBuffer);

    Status = DeviceIoControl(g_DeviceHandle,              // Handle to device
                             IOCTL_DEBUGGER_READ_MEMORY,  // IO Control Code (IOCTL)
                             &ReadMem,                    // Input Buffer to driver.
                             SIZEOF_DEBUGGER_READ_MEMORY, // Input buffer length
                             OutputBuffer,                // Output Buffer from driver.
                             SizeOfTargetBuffer,          // Length of output buffer in bytes.
                             &ReturnedLength,             // Bytes placed in buffer.
                             NULL                         // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        std::free(OutputBuffer);
        return;
    }

    //
    // Check if reading memory was successful or not
    //
    if (((PDEBUGGER_READ_MEMORY)OutputBuffer)->KernelStatus != DEBUGGER_OPERATION_WAS_SUCCESSFUL)
    {
        std::free(OutputBuffer);
        ShowErrorMessage(((PDEBUGGER_READ_MEMORY)OutputBuffer)->KernelStatus);
        return;
    }
    else
    {
        //
        // Change the ReturnedLength as it contains the headers
        //
        ReturnedLength -= SIZEOF_DEBUGGER_READ_MEMORY;
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
                                                          ((unsigned char *)OutputBuffer) + sizeof(DEBUGGER_READ_MEMORY),
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
            ((unsigned char *)OutputBuffer) + sizeof(DEBUGGER_READ_MEMORY),
            Size,
            Address,
            MemoryType,
            ReturnedLength);

        break;

    case DEBUGGER_SHOW_COMMAND_DC:

        ShowMemoryCommandDC(
            ((unsigned char *)OutputBuffer) + sizeof(DEBUGGER_READ_MEMORY),
            Size,
            Address,
            MemoryType,
            ReturnedLength);

        break;

    case DEBUGGER_SHOW_COMMAND_DD:

        ShowMemoryCommandDD(
            ((unsigned char *)OutputBuffer) + sizeof(DEBUGGER_READ_MEMORY),
            Size,
            Address,
            MemoryType,
            ReturnedLength);

        break;

    case DEBUGGER_SHOW_COMMAND_DQ:

        ShowMemoryCommandDQ(
            ((unsigned char *)OutputBuffer) + sizeof(DEBUGGER_READ_MEMORY),
            Size,
            Address,
            MemoryType,
            ReturnedLength);

        break;

    case DEBUGGER_SHOW_COMMAND_DUMP:

        CommandDumpSaveIntoFile(((unsigned char *)OutputBuffer) + sizeof(DEBUGGER_READ_MEMORY), Size);

        break;

    case DEBUGGER_SHOW_COMMAND_DISASSEMBLE64:

        //
        // Check if assembly mismatch occurred with the target address
        //
        if (((PDEBUGGER_READ_MEMORY)OutputBuffer)->Is32BitAddress == TRUE &&
            MemoryType == DEBUGGER_READ_VIRTUAL_ADDRESS)
        {
            ShowMessages("the target address seems to be located in a 32-bit program, if so, "
                         "please consider using the 'u32' instead to utilize the 32-bit disassembler\n");
        }

        //
        // Show diassembles
        //
        HyperDbgDisassembler64(
            ((unsigned char *)OutputBuffer) + sizeof(DEBUGGER_READ_MEMORY),
            Address,
            ReturnedLength,
            0,
            FALSE,
            NULL);

        break;

    case DEBUGGER_SHOW_COMMAND_DISASSEMBLE32:

        //
        // Check if assembly mismatch occurred with the target address
        //
        if (((PDEBUGGER_READ_MEMORY)OutputBuffer)->Is32BitAddress == FALSE &&
            MemoryType == DEBUGGER_READ_VIRTUAL_ADDRESS)
        {
            ShowMessages("the target address seems to be located in a 64-bit program, if so, "
                         "please consider using the 'u' instead to utilize the 64-bit disassembler\n");
        }

        //
        // Show diassembles
        //
        HyperDbgDisassembler32(
            ((unsigned char *)OutputBuffer) + sizeof(DEBUGGER_READ_MEMORY),
            Address,
            ReturnedLength,
            0,
            FALSE,
            NULL);

        break;
    }

    //
    // free the buffer
    //
    std::free(OutputBuffer);

    ShowMessages("\n");
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
