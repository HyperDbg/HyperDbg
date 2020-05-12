/**
 * @file BreakControl.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Gets the control of debuggee and gracefully halt the system
 * @details This file uses to halt system for remote connection
 * @version 0.1
 * @date 2020-05-1
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */

#include <ntddk.h>

VOID
BreakControlPrepareToHaltTheSystem()
{
    //
    // Register NMI Handler (as we'll inject virtual nmis)
    //
}

BOOLEAN
BreakControlNmiCallbackHandler(IN PVOID   Context, IN BOOLEAN Handled)
{

}
