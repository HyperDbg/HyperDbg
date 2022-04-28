/**
 * @file VmxBroadcast.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers for broadcast in VMX-root mode
 * @details
 * @version 0.1
 * @date 2021-12-31
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//					  Enums		    			//
//////////////////////////////////////////////////

/**
 * @brief Types of actions for NMI broadcasting
 * 
 */
typedef enum _NMI_BROADCAST_ACTION_TYPE
{
    NMI_BROADCAST_ACTION_NONE = 0,
    NMI_BROADCAST_ACTION_TEST,
    NMI_BROADCAST_ACTION_KD_HALT_CORE,

} NMI_BROADCAST_ACTION_TYPE;

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

BOOLEAN
VmxBroadcastHandleNmiCallback(PVOID Context, BOOLEAN Handled);

BOOLEAN
VmxBroadcastNmi(UINT32 CurrentCoreIndex, NMI_BROADCAST_ACTION_TYPE VmxBroadcastAction);

BOOLEAN
VmxBroadcastNmiHandler(UINT32 CurrentCoreIndex, PGUEST_REGS GuestRegs, BOOLEAN IsOnVmxNmiHandler);
