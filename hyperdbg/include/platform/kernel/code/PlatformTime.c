/**
 * @file PlatformTime.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of cross platform APIs for system time operations
 * @details
 * @version 0.19
 * @date 2026-05-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/PlatformTime.h"
#    include <linux/timekeeping.h> // ktime_get_real_ts64
#    include <linux/time.h>        // time64_to_tm, struct tm, sys_tz

//
// Windows kernel time is 100-nanosecond ticks since 1601-01-01; Linux gives
// Unix-epoch (1970) seconds. These bridge the two.
//
#    define HUNDRED_NS_PER_SEC           10000000LL
#    define EPOCH_DIFF_1601_TO_1970_SECS 11644473600LL
#endif // defined(__linux__)

/**
 * @brief Query the current system time
 *
 * @param SystemTime Receives the current system time as a LARGE_INTEGER (100-nanosecond units since January 1, 1601)
 * @return VOID
 */
VOID
PlatformTimeQuerySystemTime(PLARGE_INTEGER SystemTime)
{
#if defined(_WIN32) || defined(_WIN64)

    KeQuerySystemTime(SystemTime);

#elif defined(__linux__)

    struct timespec64 Ts;

    ktime_get_real_ts64(&Ts);

    //
    // Shift Unix epoch (1970) → NT epoch (1601) and rescale sec/nsec to 100-ns.
    //
    SystemTime->QuadPart =
        ((LONGLONG)Ts.tv_sec + EPOCH_DIFF_1601_TO_1970_SECS) * HUNDRED_NS_PER_SEC + Ts.tv_nsec / 100;

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Convert system time (UTC) to local time
 *
 * @param SystemTime Pointer to the system time value in UTC
 * @param LocalTime Receives the converted local time value
 * @return VOID
 */
VOID
PlatformTimeConvertToLocalTime(PLARGE_INTEGER SystemTime, PLARGE_INTEGER LocalTime)
{
#if defined(_WIN32) || defined(_WIN64)

    ExSystemTimeToLocalTime(SystemTime, LocalTime);

#elif defined(__linux__)

    //
    // Apply the kernel's timezone bias (minutes west of UTC). sys_tz is 0 on
    // systems that keep the RTC in UTC, in which case LocalTime == SystemTime.
    //
    LocalTime->QuadPart =
        SystemTime->QuadPart - (LONGLONG)sys_tz.tz_minuteswest * 60 * HUNDRED_NS_PER_SEC;

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Convert a LARGE_INTEGER time value to a TIME_FIELDS structure
 *
 * @param Time Pointer to the time value to convert
 * @param TimeFields Receives the broken-down time fields (year, month, day, hour, minute, etc.)
 * @return VOID
 */
VOID
PlatformTimeConvertToTimeFields(PLARGE_INTEGER Time, PTIME_FIELDS TimeFields)
{
#if defined(_WIN32) || defined(_WIN64)

    RtlTimeToTimeFields(Time, TimeFields);

#elif defined(__linux__)

    struct tm Tm;
    LONGLONG  Ticks   = Time->QuadPart;
    time64_t  UnixSec = (time64_t)(Ticks / HUNDRED_NS_PER_SEC) - EPOCH_DIFF_1601_TO_1970_SECS;
    SHORT     Msec    = (SHORT)((Ticks % HUNDRED_NS_PER_SEC) / 10000);

    time64_to_tm(UnixSec, 0, &Tm);

    TimeFields->Year         = (SHORT)(Tm.tm_year + 1900); // tm_year is years since 1900
    TimeFields->Month        = (SHORT)(Tm.tm_mon + 1);     // tm_mon is 0..11
    TimeFields->Day          = (SHORT)Tm.tm_mday;
    TimeFields->Hour         = (SHORT)Tm.tm_hour;
    TimeFields->Minute       = (SHORT)Tm.tm_min;
    TimeFields->Second       = (SHORT)Tm.tm_sec;
    TimeFields->Milliseconds = Msec;
    TimeFields->Weekday      = (SHORT)Tm.tm_wday; // both count from Sunday = 0

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Read the performance counter (and, optionally, its frequency).
 * @details Windows: KeQueryPerformanceCounter(). Linux arm is a stub that
 *          reports 0 for both counter and frequency.
 *
 * TODO(Linux): back with ktime_get_ns()/tsc so busy-wait loops using this
 *              actually elapse.
 */
LARGE_INTEGER
PlatformTimeQueryPerformanceCounter(PLARGE_INTEGER PerformanceFrequency)
{
#if defined(_WIN32) || defined(_WIN64)

    return KeQueryPerformanceCounter(PerformanceFrequency);

#elif defined(__linux__)

    LARGE_INTEGER Counter;
    Counter.QuadPart = 0;

    if (PerformanceFrequency != NULL)
        PerformanceFrequency->QuadPart = 0;

    return Counter; // TODO(Linux)

#else

#    error "Unsupported platform"

#endif
}
