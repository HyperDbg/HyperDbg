/**
 * @file Broadcast.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief The broadcast (DPC) function to all the cores for debugger commands
 * @details
 * @version 0.1
 * @date 2020-04-17
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//		  Internal Broadcast Functions			//
//////////////////////////////////////////////////

VOID
BroadcastVmxVirtualizationAllCores();

VOID
BroadcastEnableNmiExitingAllCores();

VOID
BroadcastDisableNmiExitingAllCores();

VOID
BroadcastNotifyAllToInvalidateEptAllCores();

VOID
BroadcastEnablePmlOnAllProcessors();

VOID
BroadcastDisablePmlOnAllProcessors();

VOID
BroadcastChangeToMbecSupportedEptpOnAllProcessors();

VOID
BroadcastRestoreToNormalEptpOnAllProcessors();

VOID
BroadcasDisableMbecOnAllProcessors();

VOID
BroadcasEnableMbecOnAllProcessors();
