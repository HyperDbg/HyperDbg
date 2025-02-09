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

    const char * PciHeaderTypes[] = {"Endpoint", "PCI-to-PCI Bridge", "PCI-to-CardBus Bridge"};

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
        PcidevinfoPacket.DeviceInfo.Bus = (UINT8)TargetBus;
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
        PcidevinfoPacket.DeviceInfo.Device = (UINT8)TargetDevice;
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
        PcidevinfoPacket.DeviceInfo.Function = (UINT8)TargetFunction;
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
            //
            // For some reason, MSVC refuses to initialize these at top of case
            //
            const char * PciHeaderTypeAsString[]  = {"Endpoint", "PCI-to-PCI Bridge", "PCI-to-CardBus Bridge"};
            const char * PciMmioBarTypeAsString[] = {"32-bit Wide",
                                                     "Reserved",
                                                     "64-bit Wide",
                                                     "Reserved"};
            UINT8        BarNumOffset             = 0;

            ShowMessages("PCI configuration space (CAM) for device %04x:%02x:%02x:%x\n",
                         0, // TODO: Add support for domains beyond 0000
                         PcidevinfoPacket.DeviceInfo.Bus,
                         PcidevinfoPacket.DeviceInfo.Device,
                         PcidevinfoPacket.DeviceInfo.Function);

            if (!PcidevinfoPacket.PrintRaw)
            {
                Vendor * CurrentVendor     = GetVendorById(PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.VendorId);
                CHAR *   CurrentVendorName = (CHAR *)"N/A";
                CHAR *   CurrentDeviceName = (CHAR *)"N/A";

                if (CurrentVendor != NULL)
                {
                    CurrentVendorName      = CurrentVendor->VendorName;
                    Device * CurrentDevice = GetDeviceFromVendor(CurrentVendor, PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.DeviceId);

                    if (CurrentDevice != NULL)
                    {
                        CurrentDeviceName = CurrentDevice->DeviceName;
                    }
                }

                ShowMessages("\nCommon Header:\nVID:DID: %04x:%04x\nVendor Name: %-17.*s\nDevice Name: %.*s\nCommand: %04x\n",
                             PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.VendorId,
                             PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.DeviceId,
                             strnlen_s(CurrentVendorName, PCI_NAME_STR_LENGTH),
                             CurrentVendorName,
                             strnlen_s(CurrentDeviceName, PCI_NAME_STR_LENGTH),
                             CurrentDeviceName,
                             PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.Command);

                if ((PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.HeaderType & 0x01) << 7 == 0) // Only applicable to endpoints
                {
                    ShowMessages("  Memory Space: %u\n  I/O Space: %u\n",
                                 (PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.Command & 0x2) >> 1,
                                 (PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.Command & 0x1));
                }

                ShowMessages("Status: %04x\nRevision ID: %02x\nClass Code: %06x\nCacheLineSize: %02x\nPrimaryLatencyTimer: %02x\nHeaderType: %s (%02x)\n  Multi-function Device: %s\nBist: %02x\n",
                             PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.Status,
                             PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.RevisionId,
                             PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.ClassCode,
                             PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.CacheLineSize,
                             PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.PrimaryLatencyTimer,
                             (PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.HeaderType & 0x3f) < 2 ? PciHeaderTypeAsString[(PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.HeaderType & 0x1)] : "Unknown",
                             PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.HeaderType,
                             (PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.HeaderType & 0x1) ? "True" : "False",
                             PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.Bist);
                FreeVendor(CurrentVendor);
                FreePciIdDatabase();

                ShowMessages("\nDevice Header:\n");

                if ((PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.HeaderType & 0x01) << 7 == 0) // Endpoint
                {
                    for (UINT8 i = 0; i < 5; i++)
                    {
                        //
                        // Memory I/O
                        //
                        if ((PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.Bar[i] & 0x1) == 0)
                        {
                            //
                            // 64-bit BAR
                            //
                            if (((PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.Bar[i] & 0x6) >> 1) == 2)
                            {
                                UINT64 BarMsb    = PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.Bar[i + 1];
                                UINT64 BarLsb    = PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.Bar[i];
                                UINT64 ActualBar = ((BarMsb & 0xFFFFFFFF) << 32) + (BarLsb & 0xFFFFFFF0);

                                ShowMessages("BAR%u %s\n BAR Type: MMIO\n MMIO BAR Type: %s (%02x)\n BAR MSB: %08x\n BAR LSB: %08x\n BAR (actual): %016llx\n Prefetchable: %s\n",
                                             i - BarNumOffset,
                                             ((PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.Command & 0x2) >> 1 == 0) || !PcidevinfoPacket.DeviceInfo.MmioBarInfo[i].IsEnabled ? "[disabled]" : "",
                                             PciMmioBarTypeAsString[(PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.Bar[i] & 0x6) >> 1],
                                             (PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.Bar[i] & 0x6) >> 1,
                                             BarMsb,
                                             BarLsb,
                                             ActualBar,
                                             (PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.Bar[i] & 0x8 >> 3) ? "True" : "False");

                                ShowMessages(" Addressable range: 0-%016llx\n Size: %u\n", PcidevinfoPacket.DeviceInfo.MmioBarInfo[i].BarOffsetEnd, PcidevinfoPacket.DeviceInfo.MmioBarInfo[i].BarSize);

                                i++;
                                BarNumOffset++;
                            }
                            //
                            // 32-bit BAR
                            //
                            else
                            {
                                UINT32 ActualBar = (PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.Bar[i] & 0xFFFFFFF0);

                                ShowMessages("BAR%u %s\n BAR Type: MMIO\n BAR: %08x\n BAR (actual): %08x\n Prefetchable: %s\n",
                                             i - BarNumOffset,
                                             ((PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.Command & 0x2) >> 1 == 0) || !PcidevinfoPacket.DeviceInfo.MmioBarInfo[i].IsEnabled ? "[disabled]" : "",
                                             PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.Bar[i],
                                             ActualBar,
                                             (PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.Bar[i] & 0x8 >> 3) ? "True" : "False");

                                ShowMessages(" Addressable range: 0-%08x\n Size: %u\n", PcidevinfoPacket.DeviceInfo.MmioBarInfo[i].BarOffsetEnd, PcidevinfoPacket.DeviceInfo.MmioBarInfo[i].BarSize);
                            }
                        }
                        //
                        // Port I/O
                        //
                        else
                        {
                            //
                            // 32-bit BAR is the only flavor we have here
                            //
                            UINT32 ActualBar32 = PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.Bar[i] & 0xFFFFFFFC;

                            ShowMessages("BAR%u %s\n BAR Type: Port IO\n BAR: %08x\n BAR (actual): %08x\n Reserved: %u\n",
                                         i - BarNumOffset,
                                         ((PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.Command & 0x1) == 0) ? "[disabled]" : "",
                                         PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.Bar[i],
                                         ActualBar32,
                                         (PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.Bar[i] & 0x2) >> 1);
                        }
                    }

                    ShowMessages("Cardbus CIS Pointer: %08x\nSubsystem Vendor ID: %04x\nSubsystem ID: %04x\nROM BAR: %08x\nCapabilities Pointer: %02x\nReserved (0xD): %06x\nReserved (0xE): %08x\nInterrupt Line: %02x\nInterrupt Pin: %02x\nMin Grant: %02x\nMax latency: %02x\n",
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.CardBusCISPtr,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.SubVendorId,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.SubSystemId,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.ROMBar,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.CapabilitiesPtr,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.Reserved,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.Reserved1,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.InterruptLine,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.InterruptPin,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.MinGnt,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpaceEp.MaxLat);
                }
                else if ((PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.HeaderType & 0x3f) == 1) // PCI-to-PCI Bridge
                {
                    ShowMessages("BAR0: %08x\nBAR1: %08x\n", PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.Bar[0], PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.Bar[1]);

                    ShowMessages("Primary Bus Number: %02x\nSecondary Bus Number: %02x\nSubordinate Bus Number: %02x\nSecondary Latency Timer: %02x\nI/O Base: %02x\nI/O Limit: %02x\nSecondary Status: %04x\nMemory Base: %04x\nMemory Limit: %04x\nPrefetchable Memory Base: %04x\nPrefetchable Memory Limit: %04x\nPrefetchable Base Upper 32 Bits: %08x\nPrefetchable Limit Upper 32 Bits: %08x\nI/O Base Upper 16 Bits: %04x\nI/O Limit Upper 16 Bits: %04x\nCapability Pointer: %02x\nReserved: %06x\nROM BAR: %08x\nInterrupt Line: %02x\nInterrupt Pin: %02x\nBridge Control: %04x\n",
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.PrimaryBusNumber,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.SecondaryBusNumber,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.SubordinateBusNumber,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.SecondaryLatencyTimer,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.IoBase,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.IoLimit,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.SecondaryStatus,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.MemoryBase,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.MemoryLimit,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.PrefetchableMemoryBase,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.PrefetchableMemoryLimit,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.PrefetchableBaseUpper32b,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.PrefetchableLimitUpper32b,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.IoLimitUpper16b,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.IoBaseUpper16b,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.CapabilityPtr,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.Reserved,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.ROMBar,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.InterruptLine,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.InterruptPin,
                                 PcidevinfoPacket.DeviceInfo.ConfigSpace.DeviceHeader.ConfigSpacePtpBridge.BridgeControl);
                }
                else if ((PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.HeaderType & 0x3f) == 2) // PCI-to-CardBus Bridge
                {
                    ShowMessages("Parsing header type %s (%02x) currently unsupported\n", PciHeaderTypeAsString[PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.HeaderType & 0x01], PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.HeaderType & 0x01);
                }
                else
                {
                    ShowMessages("\nDevice Header:\nUnknown header type %02x\n", (PcidevinfoPacket.DeviceInfo.ConfigSpace.CommonHeader.HeaderType & 0x3f));
                }
            }
            else
            {
                UINT32 * cs = (UINT32 *)&PcidevinfoPacket.DeviceInfo.ConfigSpace; // Overflows into .ConfigSpaceAdditional - no padding due to pack(0)

                ShowMessages("    00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n");

                for (UINT16 i = 0; i < CAM_CONFIG_SPACE_LENGTH; i += 16)
                {
                    ShowMessages("%02x: ", i);
                    for (UINT8 j = 0; j < 16; j++)
                    {
                        ShowMessages("%02x ", *(((BYTE *)cs) + j));
                    }

                    //
                    // Print ASCII representation
                    // Replace non-printable characters with "."
                    //
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
