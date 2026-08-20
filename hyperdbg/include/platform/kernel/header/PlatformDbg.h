/**
 * @file PlatformDbg.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Cross platform APIs for kernel debug output
 * @details
 * @version 0.19
 * @date 2026-05-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#if defined(__linux__)
#    include "../../../../include/SDK/HyperDbgSdk.h"
#endif // defined(__linux__)

//////////////////////////////////////////////////
//                  Functions                   //
//////////////////////////////////////////////////

VOID
PlatformDbgPrint(const CHAR * Format, ...);

#if defined(__linux__)

//////////////////////////////////////////////////
//     Linux stand-ins for WDK debug APIs       //
//        (placeholder stubs, see the .c)       //
//////////////////////////////////////////////////

//
// WDK: break into the attached kernel debugger (int 3). It sits inside the
// LogError() macro, so nearly every TU expands it. On Linux it is a no-op — see
// PlatformDbg.c for why a real breakpoint would panic the box.
//
VOID
DbgBreakPoint(VOID);

#endif // defined(__linux__)
