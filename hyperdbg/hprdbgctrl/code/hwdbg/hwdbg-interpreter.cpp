/**
 * @file hwdbg-interpreter.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Interpreter of hwdbg packets and requests
 * @details
 * @version 1.0
 * @date 2024-06-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern HWDBG_INSTANCE_INFORMATION g_HwdbgInstanceInfo;
extern BOOLEAN                    g_HwdbgInstanceInfoIsValid;
extern std::vector<UINT32>        g_HwdbgPortConfiguration;
;

/**
 * @brief Interpret packets of hwdbg
 *
 * @param BufferReceived
 * @param LengthReceived
 * @return BOOLEAN
 */
BOOLEAN
HwdbgInterpretPacket(PVOID BufferReceived, UINT32 LengthReceived)
{
    PHWDBG_INSTANCE_INFORMATION InstanceInfoPacket;
    PUINT32                     InstanceInfoPorts;
    DEBUGGER_REMOTE_PACKET *    TheActualPacket = NULL;
    BOOLEAN                     Result          = FALSE;

    //
    // Apply the initial offset
    //
    if (g_HwdbgInstanceInfoIsValid)
    {
        //
        // Use the debuggee's preferred offset (area) since the instance info
        // already received and interpreted
        //
        TheActualPacket = (DEBUGGER_REMOTE_PACKET *)(((CHAR *)BufferReceived) + g_HwdbgInstanceInfo.debuggeeAreaOffset);
    }
    else
    {
        //
        // Use default initial offset as there is no information (instance info)
        // from debuggee
        //
        TheActualPacket = (DEBUGGER_REMOTE_PACKET *)(((CHAR *)BufferReceived) + DEFAULT_INITIAL_DEBUGGEE_TO_DEBUGGER_OFFSET);
    }

    if (TheActualPacket->Indicator == INDICATOR_OF_HYPERDBG_PACKET)
    {
        //
        // Check checksum (for hwdbg, checksum is ignored)
        //
        // if (KdComputeDataChecksum((PVOID)&TheActualPacket->Indicator,
        //                           LengthReceived - sizeof(BYTE)) != TheActualPacket->Checksum)
        // {
        //     ShowMessages("err, checksum is invalid\n");
        //     return FALSE;
        // }

        //
        // Check if the packet type is correct
        //
        if (TheActualPacket->TypeOfThePacket != DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER_HARDWARE_LEVEL)
        {
            //
            // sth wrong happened, the packet is not belonging to use
            // for hwdbg interpreter
            //
            ShowMessages("err, unknown packet received from the debuggee\n");
            return FALSE;
        }

        //
        // It's a HyperDbg packet (for hwdbg)
        //
        switch (TheActualPacket->RequestedActionOfThePacket)
        {
        case hwdbgResponseInvalidPacketOrError:

            Result = TRUE;

            //
            // Todo: implement it
            //

            break;

        case hwdbgResponseInstanceInfo:

            Result             = TRUE;
            InstanceInfoPacket = (HWDBG_INSTANCE_INFORMATION *)(((CHAR *)TheActualPacket) + sizeof(DEBUGGER_REMOTE_PACKET));
            InstanceInfoPorts  = (UINT32 *)(((CHAR *)InstanceInfoPacket) + sizeof(HWDBG_INSTANCE_INFORMATION));

            //
            // Copy the instance info into the global hwdbg instance info
            //
            RtlCopyMemory(&g_HwdbgInstanceInfo, InstanceInfoPacket, sizeof(HWDBG_INSTANCE_INFORMATION));

            //
            // Instance info is valid from now
            //
            g_HwdbgInstanceInfoIsValid = TRUE;

            //
            // Read port arrangements
            //
            for (size_t i = 0; i < g_HwdbgInstanceInfo.numberOfPorts; i++)
            {
                g_HwdbgPortConfiguration.push_back(InstanceInfoPorts[i]);
            }

            break;

        case hwdbgResponseScriptBufferConfigurationResult:

            Result = TRUE;

            //
            // Todo: implement it
            //

            break;

        default:

            Result = FALSE;
            ShowMessages("err, unknown packet request received from the debuggee\n");

            break;
        }
    }

    //
    // Packet handled successfully
    //
    return Result;
}
