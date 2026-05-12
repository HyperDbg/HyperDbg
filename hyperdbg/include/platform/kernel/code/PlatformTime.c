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

#    error "Not yet implemented"

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

#    error "Not yet implemented"

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

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}
