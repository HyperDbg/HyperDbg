/**
 * @file PlatformIo.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of cross platform APIs for I/O Request Packet (IRP) management
 * @details
 * @version 0.19
 * @date 2026-05-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/PlatformIo.h"
#endif // defined(__linux__)

/**
 * @brief Get the current I/O stack location from an IRP
 *
 * @param Irp Pointer to the IRP (I/O Request Packet)
 * @return PIO_STACK_LOCATION Pointer to the current stack location
 */
PIO_STACK_LOCATION
PlatformIoGetCurrentIrpStackLocation(PIRP Irp)
{
#if defined(_WIN32) || defined(_WIN64)

    return IoGetCurrentIrpStackLocation(Irp);

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Complete an IRP and release it back to the I/O manager
 *
 * @param Irp Pointer to the IRP to complete
 * @param PriorityBoost Priority boost value (e.g., IO_NO_INCREMENT)
 * @return VOID
 */
VOID
PlatformIoCompleteRequest(PIRP Irp, CCHAR PriorityBoost)
{
#if defined(_WIN32) || defined(_WIN64)

    IoCompleteRequest(Irp, PriorityBoost);

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Mark the current IRP stack location as pending
 *
 * @param Irp Pointer to the IRP to mark as pending
 * @return VOID
 */
VOID
PlatformIoMarkIrpPending(PIRP Irp)
{
#if defined(_WIN32) || defined(_WIN64)

    IoMarkIrpPending(Irp);

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}
