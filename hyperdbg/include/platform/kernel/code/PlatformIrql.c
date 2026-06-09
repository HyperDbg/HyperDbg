/**
 * @file PlatformIrql.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of cross platform APIs for IRQL management
 * @details
 * @version 0.19
 * @date 2026-05-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/PlatformIrql.h"
#endif // defined(__linux__)

/**
 * @brief Raise the current IRQL to DISPATCH_LEVEL
 *
 * @return KIRQL The previous IRQL before the raise
 */
KIRQL
PlatformIrqlRaiseToDpcLevel(VOID)
{
#if defined(_WIN32) || defined(_WIN64)

    return KeRaiseIrqlToDpcLevel();

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Lower the current IRQL to the previously saved value
 *
 * @param OldIrql The previous IRQL to restore
 * @return VOID
 */
VOID
PlatformIrqlLower(KIRQL OldIrql)
{
#if defined(_WIN32) || defined(_WIN64)

    KeLowerIrql(OldIrql);

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}
