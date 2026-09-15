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
    ShowMessages("uout : writes I/O port (port mapped I/O).\n\n");

    ShowMessages("syntax : \tuout [Register (string)] [Port (hex)] [Value (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : uout al 0x60 0xed\n");
    ShowMessages("\t\te.g : uout ax 0x3f8 0x004d\n");
    ShowMessages("\t\te.g : uout eax 0xcf8 0x80000000\n");
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

    ShowMessages("  port:          0x%04x (%d)\n", PortAddress, PortAddress);
    ShowMessages("  register:      %s\n", RegisterName);
    ShowMessages("  entered value: 0x%08x\n", Value);
}

/**
 * @brief uout command handler
 *
 * @param OutRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
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
        return KdSendUserOutPacketToDebuggee(OutRequest);
    }
    else
    {
        //
        // It's on a local debugging mode
        //
        AssertShowMessageReturnStmt(g_IsKdModuleLoaded, g_DeviceHandle, ASSERT_MESSAGE_KD_NOT_LOADED, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

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
            return FALSE;
        }

        if (OutRequest.KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
        {
            CommandShowUserOutMessage(UserChosenRegister, PortAddress, Value);

            return TRUE;
        }

        else
        {
            ShowMessages("receiving OUT instruction result was not successful\n");

            return FALSE;
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
                ShowMessages("err, invalid cpu register, please use `al`, `ax`, or `eax` (case-insensitive)\n\n");
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
        ShowMessages("err, missing required parameters\n");
        CommandUserOutHelp();
        return;
    }

    //
    // use OUT instruction
    //
    CommandUserOutRequest(OutRequest);
}
