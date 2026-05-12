/**
 * @file PlatformIrql.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Cross platform APIs for IRQL (Interrupt Request Level) management
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

#if defined(_WIN32) || defined(_WIN64)

KIRQL
PlatformIrqlRaiseToDpcLevel(VOID);

VOID
PlatformIrqlLower(KIRQL OldIrql);

#endif // defined(_WIN32) || defined(_WIN64)
