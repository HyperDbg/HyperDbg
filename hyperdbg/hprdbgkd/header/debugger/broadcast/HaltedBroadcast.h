/**
 * @file HaltedBroadcast.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers for broadcasting functions in case of halted cores
 *
 * @version 0.7
 * @date 2023-10-19
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				    Functions					//
//////////////////////////////////////////////////

VOID
HaltedBroadcastChangeAllMsrBitmapReadAllCores(UINT64 BitmapMask);

VOID
HaltedBroadcastChangeAllMsrBitmapWriteAllCores(UINT64 BitmapMask);

VOID
HaltedBroadcastChangeAllIoBitmapAllCores(UINT64 Port);

VOID
HaltedBroadcastEnableRdpmcExitingAllCores();

VOID
HaltedBroadcastEnableRdtscExitingAllCores();
