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

VOID
PlatformTimeQuerySystemTime(PLARGE_INTEGER SystemTime);

VOID
PlatformTimeConvertToLocalTime(PLARGE_INTEGER SystemTime, PLARGE_INTEGER LocalTime);

VOID
PlatformTimeConvertToTimeFields(PLARGE_INTEGER Time, PTIME_FIELDS TimeFields);

//
// Read the performance counter (and, optionally, its frequency). Windows:
// KeQueryPerformanceCounter(). Linux: stub returning 0 for now. See PlatformTime.c.
//
LARGE_INTEGER
PlatformTimeQueryPerformanceCounter(PLARGE_INTEGER PerformanceFrequency);
