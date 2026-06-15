/**
 * @file Environment.h
 * @author Behrooz Abbassi (BehroozAbbassi@hyperdbg.org)
 * @brief The running environment of HyperDbg
 * @details
 * @version 0.1
 * @date 2022-01-17
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				    Definitions	        		//
//////////////////////////////////////////////////

//
// Check for platform
//
#if defined(_WIN32) || defined(_WIN64)
#    define HYPERDBG_ENV_WINDOWS
#elif defined(__linux__)
// #    error "This code cannot compile on Linux yet"
#    define HYPERDBG_ENV_LINUX
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
#    error "This code cannot compile on BSD yet"
#    define HYPERDBG_ENV_BSD
#else
#    error "This code cannot compile on non-Windows, non-Linux, and non-BSD platforms"
#endif

#ifdef HYPERDBG_ENV_LINUX

// SAL annotations
#    define _In_
#    define _Out_
#    define _Inout_
#    define _In_opt_
#    define _Out_opt_
#    define _In_z_
#    define _Outptr_
#    define _In_reads_bytes_(x)
#    define _Out_writes_bytes_(x)
#    define _Inout_updates_bytes_all_(x)

// wchar_t is a C++ built-in but needs this header in C
#    include <wchar.h>

// Windows string/char types
typedef char         TCHAR;
typedef char *       LPTSTR;
typedef const char * LPCTSTR;
typedef const char * LPCSTR;
typedef char *       LPSTR;
typedef const char * PCSTR;
typedef char *       PSTR;
typedef short *      PWCHAR;

// Windows socket type (Linux sockets are plain int)
typedef int SOCKET;
#    define INVALID_SOCKET ((SOCKET)(-1))
#    define SOCKET_ERROR   (-1)

// Windows calling convention (no-op on Linux)
#    define WINAPI

// Windows module handle (equivalent to dlopen's void * on Linux)
typedef void * HMODULE;

// Misc Windows macros
#    define UNREFERENCED_PARAMETER(P) ((void)(P))

// Win32 wait/event constants (used by the cross-platform sync wrappers)
#    define INFINITE      0xFFFFFFFF
#    define WAIT_OBJECT_0 0x00000000

// Win32 invalid handle sentinel (returned by the cross-platform file/serial wrappers)
#    define INVALID_HANDLE_VALUE ((HANDLE)(SIZE_T)-1)

#endif // HYPERDBG_ENV_LINUX
