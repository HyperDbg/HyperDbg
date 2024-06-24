/**
 * @file export.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief headers for controller of the reversing machine's module
 * @details
 * @version 1.0
 * @date 2024-06-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//            	    Functions                   //
//////////////////////////////////////////////////

//
// Some functions are exported to HyperDbgLibImports.h
//

VOID
ConnectLocalDebugger();

BOOLEAN
ConnectRemoteDebugger(const CHAR * Ip, const CHAR * Port);
