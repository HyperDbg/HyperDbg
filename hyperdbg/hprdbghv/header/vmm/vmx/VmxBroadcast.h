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
//					Functions					//
//////////////////////////////////////////////////

VOID
VmxBroadcastInitialize();

VOID
VmxBroadcastUninitialize();

BOOLEAN
VmxBroadcastHandleNmiCallback(PVOID Context, BOOLEAN Handled);

BOOLEAN
VmxBroadcastNmi(VIRTUAL_MACHINE_STATE * VCpu, NMI_BROADCAST_ACTION_TYPE VmxBroadcastAction);

BOOLEAN
VmxBroadcastNmiHandler(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN IsOnVmxNmiHandler);
