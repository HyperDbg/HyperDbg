/**
 * @file readmem.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
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
 */
void
HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_MEMORY_STYLE Style,
                                 UINT64                     Address,
                                 DEBUGGER_READ_MEMORY_TYPE  MemoryType,
                                 DEBUGGER_READ_READING_TYPE ReadingType,
                                 UINT32                     Pid,
                                 UINT                       Size)
{
    BOOL                 Status;
    ULONG                ReturnedLength;
    DEBUGGER_READ_MEMORY ReadMem = {0};
    UINT32               OperationCode;
    CHAR                 Character;

    ReadMem.Address     = Address;
    ReadMem.Pid         = Pid;
    ReadMem.Size        = Size;
    ReadMem.MemoryType  = MemoryType;
    ReadMem.ReadingType = ReadingType;
    ReadMem.Style       = Style;

    //
    // send the request
    //
    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        KdSendReadMemoryPacketToDebuggee(&ReadMem);
        return;
    }
    else if (!g_DeviceHandle)
    {
        ShowMessages("Handle not found, probably the driver is not loaded. Did you "
                     "use 'load' command?\n");
        return;
    }

    //
    // allocate buffer for transfering messages
    //
    unsigned char * OutputBuffer = (unsigned char *)malloc(Size);

    ZeroMemory(OutputBuffer, Size);

    Status = DeviceIoControl(g_DeviceHandle,              // Handle to device
                             IOCTL_DEBUGGER_READ_MEMORY,  // IO Control code
                             &ReadMem,                    // Input Buffer to driver.
                             SIZEOF_DEBUGGER_READ_MEMORY, // Input buffer length
                             OutputBuffer,                // Output Buffer from driver.
                             Size,                        // Length of output buffer in bytes.
                             &ReturnedLength,             // Bytes placed in buffer.
                             NULL                         // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        free(OutputBuffer);
        return;
    }

    if (Style == DEBUGGER_SHOW_COMMAND_DB)
    {
        ShowMemoryCommandDB(OutputBuffer, Size, Address, MemoryType, ReturnedLength);
    }
    else if (Style == DEBUGGER_SHOW_COMMAND_DC)
    {
        ShowMemoryCommandDC(OutputBuffer, Size, Address, MemoryType, ReturnedLength);
    }
    else if (Style == DEBUGGER_SHOW_COMMAND_DD)
    {
        ShowMemoryCommandDD(OutputBuffer, Size, Address, MemoryType, ReturnedLength);
    }
    else if (Style == DEBUGGER_SHOW_COMMAND_DQ)
    {
        ShowMemoryCommandDQ(OutputBuffer, Size, Address, MemoryType, ReturnedLength);
    }
    else if (Style == DEBUGGER_SHOW_COMMAND_DISASSEMBLE64)
    {
        HyperDbgDisassembler64(OutputBuffer, Address, ReturnedLength, 0, FALSE, NULL);
    }
    else if (Style == DEBUGGER_SHOW_COMMAND_DISASSEMBLE32)
    {
        HyperDbgDisassembler32(OutputBuffer, Address, ReturnedLength, 0, FALSE, NULL);
    }

    //
    // free the buffer
    //
    free(OutputBuffer);

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
ShowMemoryCommandDB(unsigned char * OutputBuffer, UINT Size, UINT64 Address, DEBUGGER_READ_MEMORY_TYPE MemoryType, UINT64 Length)
{
    CHAR Character;

    for (int i = 0; i < Size; i += 16)
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
ShowMemoryCommandDC(unsigned char * OutputBuffer, UINT Size, UINT64 Address, DEBUGGER_READ_MEMORY_TYPE MemoryType, UINT64 Length)
{
    CHAR Character;
    for (int i = 0; i < Size; i += 16)
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
ShowMemoryCommandDD(unsigned char * OutputBuffer, UINT Size, UINT64 Address, DEBUGGER_READ_MEMORY_TYPE MemoryType, UINT64 Length)
{
    CHAR Character;
    for (int i = 0; i < Size; i += 16)
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
ShowMemoryCommandDQ(unsigned char * OutputBuffer, UINT Size, UINT64 Address, DEBUGGER_READ_MEMORY_TYPE MemoryType, UINT64 Length)
{
    for (int i = 0; i < Size; i += 16)
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
