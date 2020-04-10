/**
 * @file Broadcast.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Broadcast debugger function to all logical cores
 * @details This file uses DPC to run its functions on all logical cores
 * @version 0.1
 * @date 2020-04-10
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */

#include "DebuggerCommands.h"

/**
 * @brief Broadcast syscall hook to all cores
 * 
 * @return VOID 
 */
VOID
BroadcastSyscallCallHookForAllCores()
{
}