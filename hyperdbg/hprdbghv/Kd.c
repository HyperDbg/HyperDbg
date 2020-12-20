/**
 * @file Kd.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Routines related to kernel debugging
 * @details 
 * @version 0.1
 * @date 2020-12-20
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief manage system halt on vmx-root mode 
 * @return VOID 
 */
VOID
KdManageSystemHaltOnVmxRoot()
{
    while (TRUE)
    {
        UCHAR Test = NULL;
        KdHyperDbgRecvByte(&Test);
        if (Test == 'G')
        {
            break;
        }
    }
}

/**
 * @brief routines for broadcast system halt 
 * @return VOID 
 */
VOID
KdBroadcastHaltOnAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(BroadcastDpcVmExitAndHaltSystemAllCores, NULL);
}

/**
 * @brief Halt the system
 * 
 * @return VOID 
 */
VOID
KdHaltSystem(PDEBUGGER_PAUSE_PACKET_RECEIVED PausePacket)
{
    //
    // Broadcast to halt everything
    //
    KdBroadcastHaltOnAllCores();

    //
    // Set the status
    //
    PausePacket->Result = DEBUGEER_OPERATION_WAS_SUCCESSFULL;
}
