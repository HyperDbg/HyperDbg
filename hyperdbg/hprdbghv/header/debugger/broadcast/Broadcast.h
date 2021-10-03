/**
 * @file Broadcast.h
 * @author Sina Karvandi (sina@rayanfam.com)
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
//					Functions					//
//////////////////////////////////////////////////

VOID
BroadcastEnableDbAndBpExitingAllCores();

VOID
BroadcastDisableDbAndBpExitingAllCores();

VOID
BroadcastEnableBreakpointExitingOnExceptionBitmapAllCores();

VOID
BroadcastDisableBreakpointExitingOnExceptionBitmapAllCores();

VOID
BroadcastEnableNmiExitingAllCores();

VOID
BroadcastDisableNmiExitingAllCores();
