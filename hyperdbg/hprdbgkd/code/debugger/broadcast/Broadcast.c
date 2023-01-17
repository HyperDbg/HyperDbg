/**
 * @file Broadcast.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Broadcast debugger function to all logical cores
 * @details This file uses DPC to run its functions on all logical cores
 * @version 0.1
 * @date 2020-04-10
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief routines to broadcast virtualization and vmx initialization
*  on all cores 
*
* @return VOID 
 */
VOID
BroadcastVmxVirtualizationAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutinePerformVirtualization, NULL);
}

/**
 * @brief routines to set vm-exit on all #DBs and #BP on all cores 
*
* @return VOID 
 */
VOID
BroadcastEnableDbAndBpExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineEnableDbAndBpExitingOnAllCores, NULL);
}

/**
 * @brief routines to unset vm-exit on all #DBs and #BP on all cores 
 *
 * @return VOID 
 */
VOID
BroadcastDisableDbAndBpExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineDisableDbAndBpExitingOnAllCores, NULL);
}

/**
 * @brief routines to enable vm-exit for breakpoints (exception bitmap) 
 *
 * @return VOID 
 */
VOID
BroadcastEnableBreakpointExitingOnExceptionBitmapAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineEnableBreakpointOnExceptionBitmapOnAllCores, NULL);
}

/**
 * @brief routines to disable vm-exit for breakpoints (exception bitmap) 
*
* @return VOID 
 */
VOID
BroadcastDisableBreakpointExitingOnExceptionBitmapAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineDisableBreakpointOnExceptionBitmapOnAllCores, NULL);
}

/**
 * @brief routines to set vm-exit on all NMIs on all cores 
 *
 * @return VOID 
 */
VOID
BroadcastEnableNmiExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineEnableNmiVmexitOnAllCores, NULL);
}

/**
 * @brief routines to set vm-exit on all NMIs on all cores 
 *
 * @return VOID 
 */
VOID
BroadcastDisableNmiExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineDisableNmiVmexitOnAllCores, NULL);
}

/**
 * @brief routines to notify to invalidate their ept on all cores 
 *
 * @return VOID 
 */
VOID
BroadcastNotifyAllToInvalidateEptAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineInvalidateEptOnAllCores, g_EptState->EptPointer.AsUInt);
}
