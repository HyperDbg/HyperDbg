/**
 * @file windows-privilege.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Windows only APIs for adjusting privileges
 * @details
 * @version 0.19
 * @date 2026-05-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#if defined(__linux__)
#    include "../../../../include/SDK/HyperDbgSdk.h"
#endif // defined(__linux__)

//////////////////////////////////////////////////
//                   Functions                  //
//////////////////////////////////////////////////

BOOLEAN
WindowsSetDebugPrivilege();
