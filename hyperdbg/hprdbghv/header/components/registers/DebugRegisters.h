/**
 * @file DebugRegisters.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for debug registers implementation
 * @details
 *
 * @version 0.1
 * @date 2021-11-23
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				   	Functions	     			//
//////////////////////////////////////////////////

BOOLEAN
DebugRegistersSet(UINT32 DebugRegNum, DEBUG_REGISTER_TYPE ActionType, BOOLEAN ApplyToVmcs, UINT64 TargetAddress);
