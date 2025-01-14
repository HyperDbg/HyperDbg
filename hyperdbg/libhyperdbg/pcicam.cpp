/**
 * @file pcicam.cpp
 * @author Björn Ruytenberg (bjorn@bjornweb.nl)
 * @brief !pcicam command
 * @details
 * @version 0.13
 * @date 2024-12-17
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
 * @brief !pcicam command help
 *
 * @return VOID
 */
VOID
CommandPcicamHelp()
{
    ShowMessages("!pcicam : dumps the PCI configuration space (CAM) for a given device.\n\n");

    ShowMessages("syntax : \t!pcicam [Bus (hex)] [Device (hex)] [Function] [Verbose]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !pciecam 0 2 0\n");
    ShowMessages("\t\te.g : !pciecam 3 0 0 v\n");
}

/**
 * @brief !pcicam command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandPcicam(vector<CommandToken> CommandTokens, string Command)
{
    BOOL                                        Status;
    ULONG                                       ReturnedLength;
    DEBUGGEE_PCIDEVINFO_REQUEST_RESPONSE_PACKET PcidevinfoPacket = {0};
    UINT32                                      TargetBus        = 0;
    UINT32                                      TargetDevice     = 0;
    UINT32                                      TargetFunction   = 0;

    if (CommandTokens.size() < 4 || CommandTokens.size() > 5)
    {
        ShowMessages("Incorrect use of '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandPcicamHelp();
        return;
    }

    if (ConvertTokenToUInt32(CommandTokens.at(1), &TargetBus))
    {
        if (TargetBus > BUS_MAX_NUM)
        {
            ShowMessages("Invalid bus number '%s'\n\n",
                         GetCaseSensitiveStringFromCommandToken(CommandTokens.at(1)).c_str());
            CommandPcicamHelp();
            return;
        }
        PcidevinfoPacket.Endpoint.Bus = (UINT8)TargetBus;
    }

    if (ConvertTokenToUInt32(CommandTokens.at(2), &TargetDevice))
    {
        if (TargetDevice > DEVICE_MAX_NUM)
        {
            ShowMessages("Invalid device number '%s'\n\n",
                         GetCaseSensitiveStringFromCommandToken(CommandTokens.at(2)).c_str());
            CommandPcicamHelp();
            return;
        }
        PcidevinfoPacket.Endpoint.Device = (UINT8)TargetDevice;
    }

    if (ConvertTokenToUInt32(CommandTokens.at(3), &TargetFunction))
    {
        if (TargetFunction > FUNCTION_MAX_NUM)
        {
            ShowMessages("Invalid function number '%s'\n\n",
                         GetCaseSensitiveStringFromCommandToken(CommandTokens.at(3)).c_str());
            CommandPcicamHelp();
            return;
        }
        PcidevinfoPacket.Endpoint.Function = (UINT8)TargetFunction;
    }

    if (CommandTokens.size() == 5)
    {
        if (strncmp(GetCaseSensitiveStringFromCommandToken(CommandTokens.at(4)).c_str(), "v", 1) == 0)
        {
            PcidevinfoPacket.PrintRaw = TRUE;
        }
        else
        {
            ShowMessages("Invalid parameter '%s'\n\n",
                         GetCaseSensitiveStringFromCommandToken(CommandTokens.at(4)).c_str());
            CommandPcicamHelp();
            return;
        }
    }

    //
    // Send buffer
    //
    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        KdSendPcidevinfoPacketToDebuggee(&PcidevinfoPacket);
    }
    else
    {
        AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturn);

        //
        // Send IOCTL
        //
        Status = DeviceIoControl(
            g_DeviceHandle,                                     // Handle to device
            IOCTL_PCIDEVINFO_ENUM,                              // IO Control Code (IOCTL)
            &PcidevinfoPacket,                                  // Input Buffer to driver.
            SIZEOF_DEBUGGEE_PCIDEVINFO_REQUEST_RESPONSE_PACKET, // Input buffer length
            &PcidevinfoPacket,                                  // Output Buffer from driver.
            SIZEOF_DEBUGGEE_PCIDEVINFO_REQUEST_RESPONSE_PACKET, // Length of output
                                                                // buffer in bytes.
            &ReturnedLength,                                    // Bytes placed in buffer.
            NULL                                                // synchronous call
        );

        if (!Status)
        {
            ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
            return;
        }

        if (PcidevinfoPacket.KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
        {
            ShowMessages("PCI configuration space (CAM) for device %04x:%02x:%02x:%x\n",
                         0, // TODO: Add support for domains beyond 0000
                         PcidevinfoPacket.Endpoint.Bus,
                         PcidevinfoPacket.Endpoint.Device,
                         PcidevinfoPacket.Endpoint.Function);

            if (!PcidevinfoPacket.PrintRaw)
            {
                Vendor * CurrentVendor     = GetVendorById(PcidevinfoPacket.Endpoint.ConfigSpace.CommonHeader.VendorId);
                CHAR *   CurrentVendorName = (CHAR *)"N/A";
                CHAR *   CurrentDeviceName = (CHAR *)"N/A";

                if (CurrentVendor != NULL)
                {
                    CurrentVendorName      = CurrentVendor->VendorName;
                    Device * CurrentDevice = GetDeviceFromVendor(CurrentVendor, PcidevinfoPacket.Endpoint.ConfigSpace.CommonHeader.DeviceId);

                    if (CurrentDevice != NULL)
                    {
                        CurrentDeviceName = CurrentDevice->DeviceName;
                    }
                }

                ShowMessages("Configuration Space\nVID:DID: %04x:%04x\nVendor Name: %-17.*s\nDevice Name: %.*s\n",
                             PcidevinfoPacket.Endpoint.ConfigSpace.CommonHeader.VendorId,
                             PcidevinfoPacket.Endpoint.ConfigSpace.CommonHeader.DeviceId,
                             strnlen_s(CurrentVendorName, PCI_NAME_STR_LENGTH),
                             CurrentVendorName,
                             strnlen_s(CurrentDeviceName, PCI_NAME_STR_LENGTH),
                             CurrentDeviceName);

                ShowMessages("Command: %04x\nStatus: %04x\nRevision ID: %02x\nClass Code: %06x\nCacheLineSize: %02x\nPrimaryLatencyTimer: %02x\nHeaderType: %02x\nBist: %02x\n",
                             PcidevinfoPacket.Endpoint.ConfigSpace.CommonHeader.Command,
                             PcidevinfoPacket.Endpoint.ConfigSpace.CommonHeader.Status,
                             PcidevinfoPacket.Endpoint.ConfigSpace.CommonHeader.RevisionId,
                             PcidevinfoPacket.Endpoint.ConfigSpace.CommonHeader.ClassCode,
                             PcidevinfoPacket.Endpoint.ConfigSpace.CommonHeader.CacheLineSize,
                             PcidevinfoPacket.Endpoint.ConfigSpace.CommonHeader.PrimaryLatencyTimer,
                             PcidevinfoPacket.Endpoint.ConfigSpace.CommonHeader.HeaderType,
                             PcidevinfoPacket.Endpoint.ConfigSpace.CommonHeader.Bist);
                FreeVendor(CurrentVendor);
                FreePciIdDatabase();
            }
            else
            {
                DWORD * cs = (DWORD *)&PcidevinfoPacket.Endpoint.ConfigSpace; // Overflows into .ConfigSpaceAdditional - no padding due to pack(0)

                ShowMessages("    00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n");

                for (UINT16 i = 0; i < CAM_CONFIG_SPACE_LENGTH; i += 16)
                {
                    ShowMessages("%02x: ", i);
                    for (UINT8 j = 0; j < 16; j++)
                    {
                        ShowMessages("%02x ", *(((BYTE *)cs) + j));
                    }

                    // Print ASCII representation
                    // Replace non-printable characters with "."
                    for (UINT8 j = 0; j < 16; j++)
                    {
                        char c = (char)*(cs + j);
                        if (c >= 32 && c <= 126)
                        {
                            ShowMessages("%c", c);
                        }
                        else
                        {
                            ShowMessages(".");
                        }
                    }
                    ShowMessages("\n");
                    cs += 4;
                }
            }
        }
        else
        {
            //
            // An err occurred, no results
            //
            ShowMessages("An err occured, no results:");

            ShowErrorMessage(PcidevinfoPacket.KernelStatus);
        }
    }
}
