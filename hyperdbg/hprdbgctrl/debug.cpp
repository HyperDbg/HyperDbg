/**
 * @file debug.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
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
 * @brief help of .debug command
 *
 * @return VOID
 */
VOID
CommandDebugHelp()
{
    ShowMessages(
        ".debug : debug target machine or make this machine a debuggee.\n\n");
    ShowMessages(
        "syntax : \t.debug [action (remote | prepare | close)] [type (serial | "
        "namedpipe)] [baud rate (decimal value)] address \n");
    ShowMessages("\t\te.g : .debug remote serial 115200 com3\n");
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
 * @brief Check if COM port is valid or not
 *
 * @param ComPort
 * @return BOOLEAN
 */
BOOLEAN
CommandDebugCheckComPort(string ComPort, UINT32 * Port)
{
    if (!ComPort.compare("com1"))
    {
        *Port = COM1_PORT;
        return TRUE;
    }
    else if (!ComPort.compare("com2"))
    {
        *Port = COM2_PORT;
        return TRUE;
    }
    else if (!ComPort.compare("com3"))
    {
        *Port = COM3_PORT;
        return TRUE;
    }
    else if (!ComPort.compare("com4"))
    {
        *Port = COM4_PORT;
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief .debug command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandDebug(vector<string> SplittedCommand, string Command)
{
    UINT32 Baudrate;
    UINT32 Port;

    if (SplittedCommand.size() == 2 && !SplittedCommand.at(1).compare("close"))
    {
        if (!g_IsSerialConnectedToRemoteDebuggee &&
            !g_IsSerialConnectedToRemoteDebugger)
        {
            ShowMessages(
                "err, debugger is not attached to any instance of debuggee\n");
        }
        else
        {
            KdCloseConnection();
        }
        return;
    }
    else if (SplittedCommand.size() <= 3)
    {
        ShowMessages("incorrect use of '.debug'\n\n");
        CommandDebugHelp();
        return;
    }

    //
    // Check the main command
    //
    if (!SplittedCommand.at(1).compare("remote"))
    {
        //
        // in the case of 'remote'
        //

        if (!SplittedCommand.at(2).compare("serial"))
        {
            //
            // Connect to a remote serial device
            //
            if (SplittedCommand.size() != 5)
            {
                ShowMessages("incorrect use of '.debug'\n\n");
                CommandDebugHelp();
                return;
            }

            //
            // Set baudrate
            //
            if (!IsNumber(SplittedCommand.at(3)))
            {
                //
                // Unkonwn parameter
                //
                ShowMessages("Unknown parameter '%s'\n\n",
                             SplittedCommand.at(3).c_str());
                CommandDebugHelp();
                return;
            }

            Baudrate = stoi(SplittedCommand.at(3));

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
            if (!CommandDebugCheckComPort(SplittedCommand.at(4), &Port))
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
            KdPrepareAndConnectDebugPort(SplittedCommand.at(4).c_str(), Baudrate, Port, FALSE, FALSE);
        }
        else if (!SplittedCommand.at(2).compare("namedpipe"))
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
            KdPrepareAndConnectDebugPort(Token.c_str(), NULL, NULL, FALSE, TRUE);
        }
        else
        {
            //
            // Unkonwn parameter
            //
            ShowMessages("Unknown parameter '%s'\n\n", SplittedCommand.at(2).c_str());
            CommandDebugHelp();
            return;
        }
    }
    else if (!SplittedCommand.at(1).compare("prepare"))
    {
        if (SplittedCommand.size() != 5)
        {
            ShowMessages("incorrect use of '.debug'\n\n");
            CommandDebugHelp();
            return;
        }

        //
        // in the case of 'prepare'
        // currently we only support serial
        //
        if (!SplittedCommand.at(2).compare("serial"))
        {
            //
            // Set baudrate
            //
            if (!IsNumber(SplittedCommand.at(3)))
            {
                //
                // Unkonwn parameter
                //
                ShowMessages("Unknown parameter '%s'\n\n",
                             SplittedCommand.at(3).c_str());
                CommandDebugHelp();
                return;
            }

            Baudrate = stoi(SplittedCommand.at(3));

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
            if (!CommandDebugCheckComPort(SplittedCommand.at(4), &Port))
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
            KdPrepareAndConnectDebugPort(SplittedCommand.at(4).c_str(), Baudrate, Port, TRUE, FALSE);
        }
        else
        {
            ShowMessages("invalid parameter '%s'\n\n", SplittedCommand.at(2));
            CommandDebugHelp();
            return;
        }
    }
    else
    {
        ShowMessages("invalid parameter '%s'\n\n", SplittedCommand.at(1));
        CommandDebugHelp();
        return;
    }
}
