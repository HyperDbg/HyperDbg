/**
 * @file pcitree.cpp
 * @author Björn Ruytenberg (bjorn@bjornweb.nl)
 * @brief !pcitree command
 * @details
 * @version 0.10.3
 * @date 2024-10-31
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
 * @brief help of the !pcitree command
 *
 * @return VOID
 */
VOID
CommandPcitreeHelp()
{
    ShowMessages("!pcitree : enumerates all PCIe endpoints on the debuggee.\n\n");

    ShowMessages("syntax : \t!pcietree\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !pcietree\n");
}

/**
 * @brief !pcitree command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandPcitree(vector<CommandToken> CommandTokens, string Command)
{
    BOOL                                     Status;
    ULONG                                    ReturnedLength;
    DEBUGGEE_PCITREE_REQUEST_RESPONSE_PACKET PcitreePacket = {0};

    if (CommandTokens.size() != 1)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandPcitreeHelp();
        return;
    }

    //
    // Prepare the buffer
    //

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Send the request over serial kernel debugger
        //
        KdSendPcitreePacketToDebuggee(&PcitreePacket);
    }
    else
    {
        AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturn);

        ShowMessages("Sending IOCTL\n");

        //
        // Send IOCTL
        //
        Status = DeviceIoControl(
            g_DeviceHandle,                                  // Handle to device
            IOCTL_PCIE_ENDPOINT_ENUM,                        // IO Control Code (IOCTL)
            &PcitreePacket,                                  // Input Buffer to driver.
            SIZEOF_DEBUGGEE_PCITREE_REQUEST_RESPONSE_PACKET, // Input buffer length
            &PcitreePacket,                                  // Output Buffer from driver.
            SIZEOF_DEBUGGEE_PCITREE_REQUEST_RESPONSE_PACKET, // Length of output
                                                             // buffer in bytes.
            &ReturnedLength,                                 // Bytes placed in buffer.
            NULL                                             // synchronous call
        );

        ShowMessages("Done sending IOCTL\n");

        if (!Status)
        {
            ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
            return;
        }

        if (PcitreePacket.KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
        {
            //
            // Show the results
            //
            ShowMessages("Result: (0000:00:00) VID:DID: %x:%x\n", PcitreePacket.PciTree.Domain[0].Bus[0].Device[0].ConfigSpace->CommonHeader.VendorId, PcitreePacket.PciTree.Domain[0].Bus[0].Device[0].ConfigSpace->CommonHeader.DeviceId);
        }
        else
        {
            //
            // An err occurred, no results
            //
            ShowMessages("An err occured, no results:");

            ShowErrorMessage(PcitreePacket.KernelStatus);
        }
    }
}
