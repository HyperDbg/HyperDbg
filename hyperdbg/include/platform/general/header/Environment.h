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

// POSIX sleep primitives (usleep) backing the Win32 Sleep() shim below
#    include <unistd.h>

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
#    define NTAPI

// Windows module handle (equivalent to dlopen's void * on Linux)
typedef void * HMODULE;

// Misc Windows macros
#    define UNREFERENCED_PARAMETER(P) ((void)(P))

// Win32 wait/event constants (used by the cross-platform sync wrappers)
#    define INFINITE      0xFFFFFFFF
#    define WAIT_OBJECT_0 0x00000000

// Win32 invalid handle sentinel (returned by the cross-platform file/serial wrappers)
#    define INVALID_HANDLE_VALUE ((HANDLE)(SIZE_T)-1)

// Win32 console-control event codes (kept at their Windows values so the
// shared BreakController() switch compiles unchanged). On Linux these are
// produced by the platform-signal layer from POSIX signals.
#    define CTRL_C_EVENT        0
#    define CTRL_BREAK_EVENT    1
#    define CTRL_CLOSE_EVENT    2
#    define CTRL_LOGOFF_EVENT   5
#    define CTRL_SHUTDOWN_EVENT 6

// Win32 process access rights / creation flags / exit-code sentinel, kept at
// their Windows values so the user-debugger process call sites compile
// unchanged. The underlying process wrappers are stubbed on Linux for now.
#    define PROCESS_TERMINATE                 0x0001
#    define PROCESS_QUERY_LIMITED_INFORMATION 0x1000
#    define CREATE_SUSPENDED                  0x00000004
#    define CREATE_NEW_CONSOLE                0x00000010
#    define STILL_ACTIVE                      0x00000103

//
// Win32 system error codes referenced by the shared device-open error handling
//
#    define ERROR_ACCESS_DENIED 5
#    define ERROR_GEN_FAILURE   31

// Win32 serial baud-rate constants (winbase.h CBR_*), kept at their Windows
// values so the shared serial-config / baud-rate-validation code compiles
// unchanged. Each constant equals its baud rate; actual Linux serial I/O is
// handled by the platform-serial layer (termios impl still TODO).
#    define CBR_110    110
#    define CBR_300    300
#    define CBR_600    600
#    define CBR_1200   1200
#    define CBR_2400   2400
#    define CBR_4800   4800
#    define CBR_9600   9600
#    define CBR_14400  14400
#    define CBR_19200  19200
#    define CBR_38400  38400
#    define CBR_56000  56000
#    define CBR_57600  57600
#    define CBR_115200 115200
#    define CBR_128000 128000
#    define CBR_256000 256000

#endif // HYPERDBG_ENV_LINUX
