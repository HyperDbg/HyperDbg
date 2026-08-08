/**
 * @file uin.cpp
 * @author Nikzad (Hossein Shirdel)
 * @brief uin command - read from i/o port address (user specified)
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
 * @brief help of the uin command
 *
 * @return VOID
 */
VOID
CommandUserInHelp()
{
    ShowMessages(
        "uin : Read from I/O port (User IN)\n\n"
        "Syntax :    uin <register> <port>\n\n"
        "Parameters :\n"
        "    register : Destination register: AL, AX, or EAX (case-insensitive)\n"
        "    port     : I/O port address (hex or decimal, 0x0000 - 0xFFFF)\n\n"
        "Examples :\n"
        "    uin al 0x60       Read a byte from keyboard port\n"
        "    uin Ax 0x3F8      Read a word from COM1\n"
        "    uin EAX 0xCF8     Read 32-bit from PCI config address\n");
}

/**
 * @brief uin command show messages
 *
 * @param UserChosenRegister
 * @param PortAddress
 * @param Data
 *
 * @return VOID
 */
VOID
CommandShowUserInMessage(USHORT UserChosenRegister,
                         USHORT PortAddress,
                         ULONG  Data)
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
    
    ShowMessages("  Port:     0x%04X (%d)\n", PortAddress, PortAddress);
    ShowMessages("  Register: %s\n", RegisterName);
    ShowMessages("  Result:   0x%08X\n", Data);
}

/**
 * @brief uin command handler
 * 
 * @param InRequest
 * 
 * @return VOID
 */
VOID
CommandUserInRequest(DEBUGGER_USER_IN_REQUEST_RESPONSE InRequest)
{
    BOOL                              Status;
    ULONG                             ReturnedLength;
    USHORT                            UserChosenRegister = InRequest.UserChosenRegister;
    USHORT                            PortAddress        = InRequest.PortAddress;

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // It's on a debugger mode
        //
        KdSendUserInPacketToDebuggee(InRequest);
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
            g_DeviceHandle,                           // Handle to device
            IOCTL_DEBUGGER_USER_IN,                   // IO Control Code (IOCTL)
            &InRequest,                               // Input Buffer to driver.
            SIZEOF_DEBUGGER_USER_IN_REQUEST_RESPONSE, // Input buffer length
            &InRequest,                               // Output Buffer from driver.
            SIZEOF_DEBUGGER_USER_IN_REQUEST_RESPONSE, // Length of output buffer in
                                                      // bytes.
            &ReturnedLength,                          // Bytes placed in buffer.
            NULL                                      // synchronous call
        );

        if (!Status)
        {
            ShowMessages("ioctl failed with code 0x%x\n", PlatformGetLastError());
            return;
        }

        if (InRequest.KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
        {
            //
            // The Data field is initialized to 0 before the ioctl call.
            // After the ioctl call, the Data field is populated, so we shouldn't define Data variable
            // before ioctl. Therefore, Data field must be defined after ioctl in order to be passed to
            // CommandShowUserInMessage function, because IOCTL will populate Data field.
            //
            ULONG Data = InRequest.Data;
            CommandShowUserInMessage(UserChosenRegister, PortAddress, Data);
        }

        else
        {
            ShowMessages("Receiving IN instruction result was not successful :(\n");
        }
    }
}

/**
 * @brief uin command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandUserIn(vector<CommandToken> CommandTokens, string Command)
{
    DEBUGGER_USER_IN_REQUEST_RESPONSE InRequest      = {};
    USHORT                            Port           = 0;
    BOOL                              SetRegister    = FALSE;
    BOOL                              SetPort        = FALSE;
    BOOLEAN                           IsFirstCommand = TRUE;

    if (CommandTokens.size() != 3)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandUserInHelp();
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
                InRequest.UserChosenRegister = AL_8_BIT_REGISTER;
            }
            else if (RegisterStr == "ax")
            {
                InRequest.UserChosenRegister = AX_16_BIT_REGISTER;
            }
            else if (RegisterStr == "eax")
            {
                InRequest.UserChosenRegister = EAX_32_BIT_REGISTER;
            }
            else
            {
                ShowMessages("invalid cpu register, please use `al`, `ax`, or `eax` (case-insensitive)\n\n");
            }

            SetRegister = TRUE;
            continue;
        }

        if (!SetPort)
        {
            //
            // Parse Port Address (second parameter)
            //
            if (!ConvertTokenToUInt16(Section, &Port))
            {
                ShowMessages("please specify a correct hex/dec value for port address\n\n");
                CommandUserInHelp();
                return;
            }

            InRequest.PortAddress = Port;

            SetPort = TRUE;
            continue;
        }
        
    }

    //
    // Check if register and port address was provided
    //
    if (!SetRegister || !SetPort)
    {
        ShowMessages("missing required parameters\n");
        ShowMessages("Usage: uin <register> <port>\n\n");
        CommandUserInHelp();
        return;
    }

    //
    // use IN instruction
    //
    CommandUserInRequest(InRequest);
}
