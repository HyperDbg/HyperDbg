/**
 * @file PlatformStr.h
 * @author Max Raulea (max.raulea@hyperdbg.org)
 * @brief Cross platform APIs for kernel string formatting
 * @details Kernel-mode counterpart of the user-mode platform-lib-calls string
 * wrappers; the function names are deliberately identical across both layers
 * @version 0.19
 * @date 2026-08-14
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

//
// VSNPRINTF
//
INT32
PlatformVsnprintf(CHAR * Buffer, SIZE_T BufferSize, const CHAR * Format, va_list ArgList);

//
// SPRINTF
//
INT32
PlatformSprintf(CHAR * Buffer, SIZE_T BufferSize, const CHAR * Format, ...);

//
// BOUNDED STRING LENGTH
//
SIZE_T
PlatformStrnlen(const CHAR * Str, SIZE_T MaxLength);

//
// WIDE-CHAR (UTF-16) STRING HELPERS
// Windows forwards to the CRT wide funcs; Linux implements them (the kernel has
// none, and WCHAR is 16-bit here via -fshort-wchar). Used by the hyperevade
// transparency code to match UTF-16 guest data.
//
SIZE_T
PlatformWcsLen(const WCHAR * String);

INT32
PlatformWcsCmp(const WCHAR * A, const WCHAR * B);

WCHAR *
PlatformWcsStr(const WCHAR * Haystack, const WCHAR * Needle);

INT32
PlatformWcsNiCmp(const WCHAR * A, const WCHAR * B, SIZE_T Count);
