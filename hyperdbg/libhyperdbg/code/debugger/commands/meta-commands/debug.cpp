/**
 * @file debug.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief .debug command
 * @details
 * @version 0.1
 * @date 2020-12-19
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern HANDLE  g_SerialListeningThreadHandle;
extern HANDLE  g_SerialRemoteComPortHandle;
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN g_IsSerialConnectedToRemoteDebugger;
extern BOOLEAN g_IsDebuggeeRunning;

/**
 * @brief help of the .debug command
 *
 * @return VOID
 */
VOID
CommandDebugHelp()
{
    ShowMessages(
        ".debug : debugs a target machine or makes this machine a debuggee.\n\n");

    ShowMessages(
        "syntax : \t.debug [remote] [serial|namedpipe] [pause] [Baudrate (decimal)] [Address (string)]\n");
    ShowMessages(
        "syntax : \t.debug [prepare] [serial] [Baudrate (decimal)] [Address (string)]\n");
    ShowMessages("syntax : \t.debug [close]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : .debug remote serial 115200 com2\n");
    ShowMessages("\t\te.g : .debug remote pause serial 115200 com2\n");
    ShowMessages("\t\te.g : .debug remote namedpipe \\\\.\\pipe\\HyperDbgPipe\n");
    ShowMessages("\t\te.g : .debug remote pause namedpipe \\\\.\\pipe\\HyperDbgPipe\n");
    ShowMessages("\t\te.g : .debug remote namedpipe \"\\\\.\\pipe\\HyperDbg Pipe\"\n");
    ShowMessages("\t\te.g : .debug prepare serial 115200 com1\n");
    ShowMessages("\t\te.g : .debug prepare serial 115200 com2\n");
    ShowMessages("\t\te.g : .debug close\n");

    ShowMessages(
        "\nvalid baud rates (decimal) : 110, 300, 600, 1200, 2400, 4800, 9600, "
        "14400, 19200, 38400, 56000, 57600, 115200, 128000, 256000\n");
    ShowMessages("valid COM ports : COM1, COM2, COM3, COM4 \n");
}

/**
 * @brief Check if COM port is valid or not
 *
 * @param ComPort
 * @return BOOLEAN
 */
BOOLEAN
CommandDebugCheckComPort(const CHAR * ComPort, UINT32 * Port)
{
    if (_stricmp(ComPort, "com1") == 0)
    {
        *Port = COM1_PORT;
        return TRUE;
    }
    else if (_stricmp(ComPort, "com2") == 0)
    {
        *Port = COM2_PORT;
        return TRUE;
    }
    else if (_stricmp(ComPort, "com3") == 0)
    {
        *Port = COM3_PORT;
        return TRUE;
    }
    else if (_stricmp(ComPort, "com4") == 0)
    {
        *Port = COM4_PORT;
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief Check if baud rate is valid or not
 *
 * @param Baudrate
 * @return BOOLEAN
 */
BOOLEAN
CommandDebugCheckBaudrate(DWORD Baudrate)
{
    if (Baudrate == CBR_110 || Baudrate == CBR_300 || Baudrate == CBR_600 ||
        Baudrate == CBR_1200 || Baudrate == CBR_2400 || Baudrate == CBR_4800 ||
        Baudrate == CBR_9600 || Baudrate == CBR_14400 || Baudrate == CBR_19200 ||
        Baudrate == CBR_38400 || Baudrate == CBR_56000 || Baudrate == CBR_57600 ||
        Baudrate == CBR_115200 || Baudrate == CBR_128000 ||
        Baudrate == CBR_256000)
    {
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief Connect to a remote serial device (Debugger)
 *
 * @param PortName
 * @param Baudrate
 * @param PauseAfterConnection
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperDbgDebugRemoteDeviceUsingComPort(const CHAR * PortName, DWORD Baudrate, BOOLEAN PauseAfterConnection)
{
    UINT32 Port;

    //
    // Check if baudrate is valid or not
    //
    if (!CommandDebugCheckBaudrate(Baudrate))
    {
        //
        // Baud-rate is invalid
        //
        return FALSE;
    }

    //
    // check if com port address is valid or not
    //
    if (!CommandDebugCheckComPort(PortName, &Port))
    {
        //
        // com port is invalid
        //
        return FALSE;
    }

    //
    // Everything is okay, connect to the remote machine to send (debugger)
    //
    return KdPrepareAndConnectDebugPort(PortName, Baudrate, Port, FALSE, FALSE, PauseAfterConnection);
}

/**
 * @brief Connect to a remote named pipe (Debugger)
 *
 * @param NamedPipe
 * @param PauseAfterConnection
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperDbgDebugRemoteDeviceUsingNamedPipe(const CHAR * NamedPipe, BOOLEAN PauseAfterConnection)
{
    return KdPrepareAndConnectDebugPort(NamedPipe, NULL, NULL, FALSE, TRUE, PauseAfterConnection);
}

/**
 * @brief Connect to a remote serial device (Debuggee)
 *
 * @param PortName
 * @param Baudrate
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperDbgDebugCurrentDeviceUsingComPort(const CHAR * PortName, DWORD Baudrate)
{
    UINT32 Port;

    //
    // Check if baudrate is valid or not
    //
    if (!CommandDebugCheckBaudrate(Baudrate))
    {
        //
        // Baud-rate is invalid
        //
        return FALSE;
    }

    //
    // check if com port address is valid or not
    //
    if (!CommandDebugCheckComPort(PortName, &Port))
    {
        //
        // com port is invalid
        //
        return FALSE;
    }

    //
    // Everything is okay, connect to the remote machine to send (debuggee)
    // Note: Pause after connect is set to FALSE, because it doesn't make sense for the
    // deubuggee to pause after connecting to the debugger
    //
    return KdPrepareAndConnectDebugPort(PortName, Baudrate, Port, TRUE, FALSE, FALSE);
}

/**
 * @brief Connect to a remote serial device (Debuggee)
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperDbgDebugCloseRemoteDebugger()
{
    //
    // Check if the debugger is attached to a debuggee
    //
    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        KdCloseConnection();
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief .debug command handler
 *
 * @param CommandTokens
 * @param Command
 * @return VOID
 */
VOID
CommandDebug(vector<CommandToken> CommandTokens, string Command)
{
    UINT32  Baudrate;
    UINT32  Port;
    BOOLEAN IsFirstCommand          = TRUE;
    BOOLEAN IsRemote                = FALSE;
    BOOLEAN IsPrepare               = FALSE;
    BOOLEAN IsSerial                = FALSE;
    BOOLEAN IsNamedPipe             = FALSE;
    BOOLEAN IsPause                 = FALSE;
    BOOLEAN IsNamedPipeAddressKnown = FALSE;
    string  NamedPipeAddress;
    BOOLEAN IsComPortAddressKnown = FALSE;
    string  ComAddress;
    BOOLEAN IsComPortBaudrateKnown = FALSE;

    if (CommandTokens.size() == 2 && CompareLowerCaseStrings(CommandTokens.at(1), "close"))
    {
        //
        // Check if the debugger is attached to a debuggee
        //
        if (!HyperDbgDebugCloseRemoteDebugger())
        {
            ShowMessages("err, debugger is not attached to any instance of debuggee\n");
        }

        return;
    }
    else if (CommandTokens.size() <= 3)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandDebugHelp();
        return;
    }

    for (auto Section : CommandTokens)
    {
        if (IsFirstCommand)
        {
            IsFirstCommand = FALSE;
            continue;
        }
        else if (!IsRemote && CompareLowerCaseStrings(Section, "remote"))
        {
            IsRemote = TRUE;
            continue;
        }
        else if (!IsPrepare && CompareLowerCaseStrings(Section, "prepare"))
        {
            IsPrepare = TRUE;
            continue;
        }
        else if (!IsSerial && CompareLowerCaseStrings(Section, "serial"))
        {
            IsSerial = TRUE;
            continue;
        }
        else if (!IsNamedPipe && CompareLowerCaseStrings(Section, "namedpipe"))
        {
            IsNamedPipe = TRUE;
            continue;
        }
        else if (!IsPause && CompareLowerCaseStrings(Section, "pause"))
        {
            IsPause = TRUE;
            continue;
        }
        else if (!IsNamedPipeAddressKnown && IsNamedPipe)
        {
            IsNamedPipeAddressKnown = TRUE;
            NamedPipeAddress        = GetCaseSensitiveStringFromCommandToken(Section);
            continue;
        }
        else if (!IsComPortBaudrateKnown && IsSerial && IsNumber(GetCaseSensitiveStringFromCommandToken(Section)))
        {
            IsComPortBaudrateKnown = TRUE;
            Baudrate               = stoi(GetCaseSensitiveStringFromCommandToken(Section));

            //
            // Check if baudrate is valid or not
            //
            if (!CommandDebugCheckBaudrate(Baudrate))
            {
                //
                // Baud-rate is invalid
                //
                ShowMessages("err, baud rate is invalid\n\n");
                CommandDebugHelp();
                return;
            }

            continue;
        }
        else if (!IsComPortAddressKnown && IsSerial)
        {
            IsComPortAddressKnown = TRUE;
            ComAddress            = GetCaseSensitiveStringFromCommandToken(Section);

            //
            // check if com port address is valid or not
            //
            if (!CommandDebugCheckComPort(ComAddress.c_str(), &Port))
            {
                //
                // com port is invalid
                //
                ShowMessages("err, COM port is invalid\n\n");
                CommandDebugHelp();
                return;
            }

            continue;
        }
        else
        {
            ShowMessages("err, couldn't resolve error at '%s'\n\n",
                         GetCaseSensitiveStringFromCommandToken(Section).c_str());
            CommandDebugHelp();
            return;
        }
    }

    //
    // Validate parameters
    //
    if (IsRemote && IsPrepare)
    {
        ShowMessages("err, both 'remote' and 'prepare' can't be used together\n\n");
        CommandDebugHelp();
        return;
    }

    if (!IsRemote && !IsPrepare)
    {
        ShowMessages("err, either 'remote' or 'prepare' should be used\n\n");
        CommandDebugHelp();
        return;
    }

    //
    // Prepare cannot be specified with the 'pause'
    //
    if (IsPrepare && IsPause)
    {
        ShowMessages("err, 'pause' cannot be used with 'prepare'\n\n");
        CommandDebugHelp();
        return;
    }

    //
    // Named pipe cannot be used with the 'prepare'
    //
    if (IsNamedPipe && IsPrepare)
    {
        ShowMessages("err, named pipe cannot be used with 'prepare'\n\n");
        CommandDebugHelp();
        return;
    }

    //
    // Check if named pipe is empty or not if it's a named pipe
    //
    if (IsNamedPipe && NamedPipeAddress.empty())
    {
        ShowMessages("err, named pipe address is empty\n\n");
        CommandDebugHelp();
        return;
    }

    //
    // If it's serial, COM address and bausrate should be known
    //
    if (IsSerial && (!IsComPortAddressKnown || !IsComPortBaudrateKnown))
    {
        ShowMessages("err, COM port address or baudrate is unknown\n\n");
        CommandDebugHelp();
        return;
    }

    //
    // Perform connecting to the remote machine or prepare to send
    //
    if (IsPrepare)
    {
        //
        // Everything is okay, prepare to send (debuggee)
        //
        HyperDbgDebugCurrentDeviceUsingComPort(ComAddress.c_str(), Baudrate);
    }
    else
    {
        //
        // Everything is okay, connect to the remote machine to send (debugger)
        //
        if (IsNamedPipe)
        {
            HyperDbgDebugRemoteDeviceUsingNamedPipe(NamedPipeAddress.c_str(), IsPause);
        }
        else if (IsSerial)
        {
            HyperDbgDebugRemoteDeviceUsingComPort(ComAddress.c_str(), Baudrate, IsPause);
        }
    }
}
