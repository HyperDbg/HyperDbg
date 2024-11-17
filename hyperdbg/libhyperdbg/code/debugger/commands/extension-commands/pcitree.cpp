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
    DEBUGGEE_PCITREE_REQUEST_RESPONSE_PACKET PcitreeReqPacket  = {0};
    DEBUGGEE_PCITREE_REQUEST_RESPONSE_PACKET PcitreeRespPacket = {0};

    if (CommandTokens.size() != 1)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandPcitreeHelp();
        return;
    }

    for (UINT8 b = 0; b < BUS_MAX_NUM; b++)
    {
        for (UINT8 d = 0; d < DEVICE_MAX_NUM; d++)
        {
            for (UINT8 f = 0; f < FUNCTION_MAX_NUM; f++)
            {
                //
                // Prepare buffer
                //
                PcitreeReqPacket.RequestedBus      = b;
                PcitreeReqPacket.RequestedDevice   = d;
                PcitreeReqPacket.RequestedFunction = f;

                //
                // Send buffer
                //
                if (g_IsSerialConnectedToRemoteDebuggee)
                {
                    KdSendPcitreePacketToDebuggee(&PcitreeReqPacket);
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
                        &PcitreeReqPacket,                               // Input Buffer to driver.
                        SIZEOF_DEBUGGEE_PCITREE_REQUEST_RESPONSE_PACKET, // Input buffer length
                        &PcitreeRespPacket,                              // Output Buffer from driver.
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

                    if (PcitreeRespPacket.KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
                    {
                        ShowMessages("Result: (%04x:%2x:%2x:%x) VID:DID: %x:%x\n",
                                     0,
                                     PcitreeRespPacket.RequestedBus,
                                     PcitreeRespPacket.RequestedDevice,
                                     PcitreeRespPacket.RequestedFunction,
                                     PcitreeRespPacket.Device.Function[0].ConfigSpace.CommonHeader.VendorId,
                                     PcitreeRespPacket.Device.Function[0].ConfigSpace.CommonHeader.DeviceId);
                    }
                    else
                    {
                        //
                        // An err occurred, no results
                        //
                        ShowMessages("An err occured, no results:");

                        ShowErrorMessage(PcitreeRespPacket.KernelStatus);
                    }
                }
            }
        }
    }
}
