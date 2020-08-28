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
#include "pch.h"

/**
 * @brief Halt the system
 * 
 * @return VOID 
 */
VOID
BreakControlPrepareToHaltTheSystem()
{
    //
    // Register NMI Handler (as we'll inject virtual nmis)
    //
}

/**
 * @brief This is the callback in the case of KeRegisterNmiCallback 
 * 
 * @param Context 
 * @param Handled 
 * @return BOOLEAN 
 */
BOOLEAN
BreakControlNmiCallbackHandler(IN PVOID Context, IN BOOLEAN Handled)
{

}
