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

#if defined(__linux__)

//////////////////////////////////////////////////
//   Linux stand-in for WDK performance counter //
//        (placeholder stub, see the .c)        //
//////////////////////////////////////////////////

LARGE_INTEGER
KeQueryPerformanceCounter(PLARGE_INTEGER PerformanceFrequency);

#endif // defined(__linux__)
