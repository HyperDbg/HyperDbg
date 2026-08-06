/**
 * @file uout.cpp
 * @author Nikzad (Hossein Shirdel)
 * @brief uout command - write from cpu register to i/o port address (user specified)
 * @details
 * @version 0.24
 * @date 2026-08-01
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_IsKdModuleLoaded;
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief help of the uout command
 *
 * @return VOID
 */
VOID
CommandUserOutHelp()
{
    ShowMessages(
        "uout : Write to I/O port (User OUT)\n\n"
        "Syntax :    uout <register> <port> <value>\n\n"
        "Parameters :\n"
        "    register : Source register: AL, AX, or EAX (case-insensitive)\n"
        "    port     : I/O port address (hex or decimal, 0x0000 - 0xFFFF)\n"
        "    value    : Value to write (hex or decimal)\n\n"
        "Examples :\n"
        "    uout al 0x60 0xED      Write to keyboard controller\n"
        "    uout AX 0x3F8 0x004D   Write word to COM1\n"
        "    uout Eax 0xCF8 0x80000000  Write 32-bit to PCI config address\n");
}

/**
 * @brief uout command show messages
 *
 * @param UserChosenRegister
 * @param PortAddress
 * @param Value
 *
 * @return VOID
 */
VOID
CommandShowUserOutMessage(USHORT UserChosenRegister,
                          USHORT PortAddress,
                          UINT32 Value)
{
    const char * RegisterName = "";

    switch (UserChosenRegister)
    {
    case AL_8_BIT_REGISTER:
        RegisterName = "AL";
        break;
    case AX_16_BIT_REGISTER:
        RegisterName = "AX";
        break;
    case EAX_32_BIT_REGISTER:
        RegisterName = "EAX";
        break;
    }

    ShowMessages("  Port:          0x%04X (%d)\n", PortAddress, PortAddress);
    ShowMessages("  Register:      %s\n", RegisterName);
    ShowMessages("  Entered Value: 0x%08X\n", Value);
}

/**
 * @brief uout command handler
 * 
 * @param OutRequest
 * 
 * @return VOID
 */
VOID
CommandUserOutRequest(DEBUGGER_USER_OUT_REQUEST_RESPONSE OutRequest)
{
    BOOL   Status;
    ULONG  ReturnedLength;
    USHORT UserChosenRegister = OutRequest.UserChosenRegister;
    USHORT PortAddress        = OutRequest.PortAddress;
    UINT32 Value              = OutRequest.Value;

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // It's on a debugger mode
        //
        KdSendUserOutPacketToDebuggee(OutRequest);
        return;
    }
    else
    {
        //
        // It's on a local debugging mode
        //
        AssertShowMessageReturnStmt(g_IsKdModuleLoaded, g_DeviceHandle, ASSERT_MESSAGE_KD_NOT_LOADED, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturn);

        //
        // By the way, we don't need to send an input buffer
        // to the kernel, but let's keep it like this, if we
        // want to pass some other arguments to the kernel in
        // the future
        //
        Status = PlatformDeviceIoControl(
            g_DeviceHandle,                            // Handle to device
            IOCTL_DEBUGGER_USER_OUT,                   // IO Control Code (IOCTL)
            &OutRequest,                               // Input Buffer to driver.
            SIZEOF_DEBUGGER_USER_OUT_REQUEST_RESPONSE, // Input buffer length
            &OutRequest,                               // Output Buffer from driver.
            SIZEOF_DEBUGGER_USER_OUT_REQUEST_RESPONSE, // Length of output buffer in
                                                       // bytes.
            &ReturnedLength,                           // Bytes placed in buffer.
            NULL                                       // synchronous call
        );

        if (!Status)
        {
            ShowMessages("ioctl failed with code 0x%x\n", PlatformGetLastError());
            return;
        }

        if (OutRequest.KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
        {
            CommandShowUserOutMessage(UserChosenRegister, PortAddress, Value);
        }

        else
        {
            ShowMessages("Receiving OUT instruction result was not successful :(\n");
        }
    }
}

/**
 * @brief uout command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandUserOut(vector<CommandToken> CommandTokens, string Command)
{
    DEBUGGER_USER_OUT_REQUEST_RESPONSE OutRequest     = {};
    USHORT                             Port           = 0;
    UINT32                             Value          = 0;
    BOOL                               SetRegister    = FALSE;
    BOOL                               SetPort        = FALSE;
    BOOL                               SetValue       = FALSE;
    BOOLEAN                            IsFirstCommand = TRUE;

    if (CommandTokens.size() != 4)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandUserOutHelp();
        return;
    }

    for (auto Section : CommandTokens)
    {
        if (IsFirstCommand == TRUE)
        {
            IsFirstCommand = FALSE;
            continue;
        }

        //
        // Parse Register (first parameter)
        //
        if (!SetRegister)
        {
            string RegisterStr = GetCaseSensitiveStringFromCommandToken(Section);

            //
            // convert to lowercase for case-insensitive comparison
            //
            transform(RegisterStr.begin(), RegisterStr.end(), RegisterStr.begin(), ::tolower);

            //
            // convert register string to enum
            //
            if (RegisterStr == "al")
            {
                OutRequest.UserChosenRegister = AL_8_BIT_REGISTER;
            }
            else if (RegisterStr == "ax")
            {
                OutRequest.UserChosenRegister = AX_16_BIT_REGISTER;
            }
            else if (RegisterStr == "eax")
            {
                OutRequest.UserChosenRegister = EAX_32_BIT_REGISTER;
            }
            else
            {
                ShowMessages("invalid cpu register, please use `al`, `ax`, or `eax` (case-insensitive)\n\n");
            }

            SetRegister = TRUE;
            continue;
        }

        //
        // Parse Port Address (second parameter)
        //
        if (!SetPort)
        {
            if (!ConvertTokenToUInt16(Section, &Port))
            {
                ShowMessages("please specify a correct hex/dec value for port address\n\n");
                CommandUserOutHelp();
                return;
            }

            OutRequest.PortAddress = Port;

            SetPort = TRUE;
            continue;
        }

        //
        // Parse Value (third parameter)
        //
        if (!SetValue)
        {
            if (!ConvertTokenToUInt32(Section, &Value))
            {
                ShowMessages("please specify a correct hex/dec value for Value\n\n");
                CommandUserOutHelp();
                return;
            }

            OutRequest.Value = Value;

            SetValue = TRUE;
            continue;
        }
    }

    //
    // Check if register, port address, and value was provided
    //
    if (!SetRegister || !SetPort || !SetValue)
    {
        ShowMessages("missing required parameters\n");
        ShowMessages("Usage: uout <register> <port> <value>\n\n");
        CommandUserOutHelp();
        return;
    }

    //
    // use OUT instruction
    //
    CommandUserOutRequest(OutRequest);
}
