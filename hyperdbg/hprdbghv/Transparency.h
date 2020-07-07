/**
 * @file Transparency.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief hide the debugger from anti-debugging and anti-hypervisor methods (headers)
 * @details
 * @version 0.1
 * @date 2020-07-07
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include <ntddk.h>

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

NTSTATUS
TransparentHideDebugger();

NTSTATUS
TransparentUnhideDebugger();
