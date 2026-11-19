/**
 * @file platform-console.h
 * @author scriptk1d
 * @brief User mode cross-platform interface for interactive console I/O
 * @details The line editor contains the editing and history logic; this
 *          interface owns the OS-specific parts: raw/canonical terminal mode,
 *          key decoding, cursor-width query, and console output.
 *
 *          Windows maps onto ReadConsoleInputW plus VT output processing.
 *          Linux maps onto termios raw mode and ANSI escape sequences.
 *
 * @version 0.1
 * @date 2026-11-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#ifdef _WIN32

#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <Windows.h>

#    ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#        define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#    endif
#    ifndef ENABLE_VIRTUAL_TERMINAL_INPUT
#        define ENABLE_VIRTUAL_TERMINAL_INPUT 0x0200
#    endif

#else

#    include <stddef.h>
#    include <string.h>
#    include "../../../../include/SDK/HyperDbgSdk.h"

#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Console key actions understood by the line editor
 */
typedef enum _PLATFORM_CONSOLE_KEY_TYPE
{
    PLATFORM_CONSOLE_KEY_NONE,
    PLATFORM_CONSOLE_KEY_CHARACTER,
    PLATFORM_CONSOLE_KEY_PASTE,
    PLATFORM_CONSOLE_KEY_ENTER,
    PLATFORM_CONSOLE_KEY_BACKSPACE,
    PLATFORM_CONSOLE_KEY_DELETE,
    PLATFORM_CONSOLE_KEY_LEFT,
    PLATFORM_CONSOLE_KEY_RIGHT,
    PLATFORM_CONSOLE_KEY_WORD_LEFT,
    PLATFORM_CONSOLE_KEY_WORD_RIGHT,
    PLATFORM_CONSOLE_KEY_HOME,
    PLATFORM_CONSOLE_KEY_END,
    PLATFORM_CONSOLE_KEY_UP,
    PLATFORM_CONSOLE_KEY_DOWN,
    PLATFORM_CONSOLE_KEY_PAGE_UP,
    PLATFORM_CONSOLE_KEY_PAGE_DOWN,
    PLATFORM_CONSOLE_KEY_KILL_TO_END,
    PLATFORM_CONSOLE_KEY_KILL_TO_START,
    PLATFORM_CONSOLE_KEY_KILL_WORD,
    PLATFORM_CONSOLE_KEY_KILL_WORD_FORWARD,
    PLATFORM_CONSOLE_KEY_YANK,
    PLATFORM_CONSOLE_KEY_TRANSPOSE,
    PLATFORM_CONSOLE_KEY_CLEAR_SCREEN,
    PLATFORM_CONSOLE_KEY_CANCEL,
    PLATFORM_CONSOLE_KEY_CTRL_D,
    PLATFORM_CONSOLE_KEY_CTRL_Z,
    PLATFORM_CONSOLE_KEY_END_OF_FILE,
    PLATFORM_CONSOLE_KEY_REVERSE_SEARCH,
    PLATFORM_CONSOLE_KEY_ABORT,
    PLATFORM_CONSOLE_KEY_RESIZE,
    PLATFORM_CONSOLE_KEY_ERROR,
} PLATFORM_CONSOLE_KEY_TYPE;

#define PLATFORM_CONSOLE_KEY_TEXT_SIZE    4
#define PLATFORM_CONSOLE_MAX_PASTE_LENGTH (1024 * 1024)

/**
 * @brief One decoded console key event
 *
 * @details Text holds one UTF-8 code point. RepeatCount is the number of
 *          times a character event should be inserted. PasteData is owned by
 *          the platform layer and stays valid until the next read or end call.
 */
typedef struct _PLATFORM_CONSOLE_KEY
{
    PLATFORM_CONSOLE_KEY_TYPE Type;
    CHAR                      Text[PLATFORM_CONSOLE_KEY_TEXT_SIZE];
    UINT32                    TextLength;
    UINT32                    RepeatCount;
    const CHAR *              PasteData;
    UINT32                    PasteLength;

} PLATFORM_CONSOLE_KEY, *PPLATFORM_CONSOLE_KEY;

#ifdef _WIN32

#    define PLATFORM_CONSOLE_NEWLINE          "\n"
#    define PLATFORM_CONSOLE_NEWLINE_LENGTH   1
#    define PLATFORM_CONSOLE_SUPPORTS_SUSPEND 0

#else

#    define PLATFORM_CONSOLE_NEWLINE          "\r\n"
#    define PLATFORM_CONSOLE_NEWLINE_LENGTH   2
#    define PLATFORM_CONSOLE_SUPPORTS_SUSPEND 1

#endif

/**
 * @brief Check whether stdin and stdout refer to an interactive terminal
 *
 * @return BOOLEAN TRUE if the line editor can take over the terminal
 */
BOOLEAN
PlatformConsoleIsInteractive(VOID);

/**
 * @brief Enter raw/interactive console mode
 *
 * @return BOOLEAN TRUE on success
 */
BOOLEAN
PlatformConsoleBegin(VOID);

/**
 * @brief Restore the original console mode
 *
 * @return VOID
 */
VOID
    PlatformConsoleEnd(VOID);

/**
 * @brief Get the current terminal width in columns
 *
 * @return UINT32 number of columns (80 when it cannot be determined)
 */
UINT32
PlatformConsoleGetColumns(VOID);

/**
 * @brief Read one decoded key event
 *
 * @param Key receives the decoded key
 *
 * @return BOOLEAN TRUE on success, FALSE on end-of-file or read error
 */
BOOLEAN
PlatformConsoleReadKey(PLATFORM_CONSOLE_KEY * Key);

/**
 * @brief Write bytes to the console and flush them
 *
 * @param Data bytes to write
 * @param Length number of bytes
 *
 * @return VOID
 */
VOID
PlatformConsoleWrite(const CHAR * Data, SIZE_T Length);

/**
 * @brief Write a platform-appropriate line ending
 *
 * @return VOID
 */
VOID
    PlatformConsoleWriteNewLine(VOID);

/**
 * @brief Return the current process id for temporary file naming
 *
 * @return UINT32 current process id
 */
UINT32
PlatformConsoleGetProcessId(VOID);

/**
 * @brief Suspend the process and resume console mode afterwards
 *
 * @return BOOLEAN TRUE if console mode was restored after resume
 */
BOOLEAN
PlatformConsoleSuspend(VOID);

#ifdef __cplusplus
}
#endif
