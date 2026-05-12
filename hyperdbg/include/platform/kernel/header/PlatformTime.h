/**
 * @file PlatformTime.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Cross platform APIs for system time operations
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

VOID
PlatformTimeQuerySystemTime(PLARGE_INTEGER SystemTime);

VOID
PlatformTimeConvertToLocalTime(PLARGE_INTEGER SystemTime, PLARGE_INTEGER LocalTime);

VOID
PlatformTimeConvertToTimeFields(PLARGE_INTEGER Time, PTIME_FIELDS TimeFields);

#endif // defined(_WIN32) || defined(_WIN64)
