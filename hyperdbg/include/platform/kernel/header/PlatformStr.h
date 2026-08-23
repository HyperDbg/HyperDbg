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
