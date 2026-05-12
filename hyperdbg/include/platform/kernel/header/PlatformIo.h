/**
 * @file PlatformIo.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Cross platform APIs for I/O Request Packet (IRP) management
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

PIO_STACK_LOCATION
PlatformIoGetCurrentIrpStackLocation(PIRP Irp);

VOID
PlatformIoCompleteRequest(PIRP Irp, CCHAR PriorityBoost);

VOID
PlatformIoMarkIrpPending(PIRP Irp);

#endif // defined(_WIN32) || defined(_WIN64)
