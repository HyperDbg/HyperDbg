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
        "syntax : \t.debug [remote] [serial|namedpipe] [Baudrate (decimal)] [Address (string)]\n");
    ShowMessages(
        "syntax : \t.debug [prepare] [serial] [Baudrate (decimal)] [Address (string)]\n");
    ShowMessages("syntax : \t.debug [close]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : .debug remote serial 115200 com2\n");
    ShowMessages("\t\te.g : .debug remote namedpipe \\\\.\\pipe\\HyperDbgPipe\n");
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
 * @brief .debug command handler
 *
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandDebug(vector<string> SplitCommand, string Command)
{
    UINT32 Baudrate;
    UINT32 Port;

    if (SplitCommand.size() == 2 && !SplitCommand.at(1).compare("close"))
    {
        //
        // Check if the debugger is attached to a debuggee
        //
        if (g_IsSerialConnectedToRemoteDebuggee)
        {
            KdCloseConnection();
        }
        else
        {
            ShowMessages(
                "err, debugger is not attached to any instance of debuggee\n");
        }
        return;
    }
    else if (SplitCommand.size() <= 3)
    {
        ShowMessages("incorrect use of the '.debug'\n\n");
        CommandDebugHelp();
        return;
    }

    //
    // Check the main command
    //
    if (!SplitCommand.at(1).compare("remote"))
    {
        //
        // in the case of the 'remote'
        //

        if (!SplitCommand.at(2).compare("serial"))
        {
            //
            // Connect to a remote serial device
            //
            if (SplitCommand.size() != 5)
            {
                ShowMessages("incorrect use of the '.debug'\n\n");
                CommandDebugHelp();
                return;
            }

            //
            // Set baudrate
            //
            if (!IsNumber(SplitCommand.at(3)))
            {
                //
                // Unknown parameter
                //
                ShowMessages("unknown parameter '%s'\n\n",
                             SplitCommand.at(3).c_str());
                CommandDebugHelp();
                return;
            }

            Baudrate = stoi(SplitCommand.at(3));

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

            //
            // check if com port address is valid or not
            //
            if (!CommandDebugCheckComPort(SplitCommand.at(4).c_str(), &Port))
            {
                //
                // com port is invalid
                //
                ShowMessages("err, COM port is invalid\n\n");
                CommandDebugHelp();
                return;
            }

            //
            // Everything is okay, connect to the remote machine to send (debugger)
            //
            HyperDbgDebugRemoteDeviceUsingComPort(SplitCommand.at(4).c_str(), Baudrate, FALSE);
        }
        else if (!SplitCommand.at(2).compare("namedpipe"))
        {
            //
            // Connect to a remote namedpipe
            //
            string Delimiter = "namedpipe";
            string Token     = Command.substr(
                Command.find(Delimiter) + Delimiter.size() + 1,
                Command.size());

            //
            // Connect to a namedpipe (it's probably a Virtual Machine debugging)
            //
            HyperDbgDebugRemoteDeviceUsingNamedPipe(Token.c_str(), FALSE);
        }
        else
        {
            //
            // Unknown parameter
            //
            ShowMessages("unknown parameter '%s'\n\n", SplitCommand.at(2).c_str());
            CommandDebugHelp();
            return;
        }
    }
    else if (!SplitCommand.at(1).compare("prepare"))
    {
        if (SplitCommand.size() != 5)
        {
            ShowMessages("incorrect use of the '.debug'\n\n");
            CommandDebugHelp();
            return;
        }

        //
        // in the case of the 'prepare'
        // currently we only support serial
        //
        if (!SplitCommand.at(2).compare("serial"))
        {
            //
            // Set baudrate
            //
            if (!IsNumber(SplitCommand.at(3)))
            {
                //
                // Unknown parameter
                //
                ShowMessages("unknown parameter '%s'\n\n",
                             SplitCommand.at(3).c_str());
                CommandDebugHelp();
                return;
            }

            Baudrate = stoi(SplitCommand.at(3));

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

            //
            // check if com port address is valid or not
            //
            if (!CommandDebugCheckComPort(SplitCommand.at(4).c_str(), &Port))
            {
                //
                // com port is invalid
                //
                ShowMessages("err, COM port is invalid\n\n");
                CommandDebugHelp();
                return;
            }

            //
            // Everything is okay, prepare to send (debuggee)
            //
            HyperDbgDebugCurrentDeviceUsingComPort(SplitCommand.at(4).c_str(), Baudrate);
        }
        else
        {
            ShowMessages("invalid parameter '%s'\n\n", SplitCommand.at(2));
            CommandDebugHelp();
            return;
        }
    }
    else
    {
        ShowMessages("invalid parameter '%s'\n\n", SplitCommand.at(1));
        CommandDebugHelp();
        return;
    }
}
