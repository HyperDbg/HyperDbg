/**
 * @file platform-console.c
 * @author scriptk1d
 * @brief User mode cross-platform implementation of interactive console I/O
 * @details See platform-console.h. The line editor owns editing/history logic;
 *          this file owns the OS-specific terminal state, key decoding, paste
 *          handling, and console output.
 *
 * @version 0.1
 * @date 2026-11-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "../header/platform-console.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

#else

#    include <errno.h>
#    include <poll.h>
#    include <signal.h>
#    include <sys/ioctl.h>
#    include <termios.h>
#    include <unistd.h>

#endif

#define PLATFORM_CONSOLE_ESCAPE_TIMEOUT_MS 100
#define PLATFORM_CONSOLE_UTF8_TIMEOUT_MS   100

#ifdef _WIN32

static HANDLE  InputHandle          = INVALID_HANDLE_VALUE;
static HANDLE  OutputHandle         = INVALID_HANDLE_VALUE;
static DWORD   OriginalInputMode    = 0;
static DWORD   OriginalOutputMode   = 0;
static BOOLEAN HaveConsoleMode      = FALSE;
static WCHAR   PendingHighSurrogate = 0;

#else

static struct termios OriginalTermios;
static BOOLEAN        HaveOriginalTermios = FALSE;
static CHAR *         PasteBuffer         = NULL;
static UINT32         PasteBufferCapacity = 0;

#endif

/**
 * @brief Return the current process id for temporary file naming
 *
 * @return UINT32 current process id
 */
UINT32
PlatformConsoleGetProcessId(VOID)
{
#ifdef _WIN32
    return (UINT32)GetCurrentProcessId();
#else
    return (UINT32)getpid();
#endif
}

/**
 * @brief Write bytes to the console and flush them
 *
 * @param Data bytes to write
 * @param Length number of bytes
 *
 * @return VOID
 */
VOID
PlatformConsoleWrite(const CHAR * Data, SIZE_T Length)
{
    if (Data == NULL || Length == 0)
    {
        return;
    }

    fwrite(Data, 1, (size_t)Length, stdout);
    fflush(stdout);
}

/**
 * @brief Write a platform-appropriate line ending
 *
 * @return VOID
 */
VOID
PlatformConsoleWriteNewLine(VOID)
{
    PlatformConsoleWrite(PLATFORM_CONSOLE_NEWLINE, PLATFORM_CONSOLE_NEWLINE_LENGTH);
}

/**
 * @brief Check whether stdin and stdout refer to an interactive terminal
 *
 * @return BOOLEAN TRUE if the line editor can take over the terminal
 */
BOOLEAN
PlatformConsoleIsInteractive(VOID)
{
#ifdef _WIN32
    HANDLE Input  = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE Output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD  Mode   = 0;

    return Input != INVALID_HANDLE_VALUE &&
           Output != INVALID_HANDLE_VALUE &&
           GetConsoleMode(Input, &Mode) &&
           GetConsoleMode(Output, &Mode);
#else
    const CHAR * Term = getenv("TERM");

    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO))
    {
        return FALSE;
    }

    if (Term != NULL && strcmp(Term, "dumb") == 0)
    {
        return FALSE;
    }

    return TRUE;
#endif
}

/**
 * @brief Enter raw/interactive console mode
 *
 * @return BOOLEAN TRUE on success
 */
BOOLEAN
PlatformConsoleBegin(VOID)
{
#ifdef _WIN32
    DWORD InputMode;
    DWORD OutputMode;

    fflush(stdout);

    InputHandle  = GetStdHandle(STD_INPUT_HANDLE);
    OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (InputHandle == INVALID_HANDLE_VALUE || OutputHandle == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    if (!GetConsoleMode(InputHandle, &OriginalInputMode))
    {
        return FALSE;
    }

    if (!GetConsoleMode(OutputHandle, &OriginalOutputMode))
    {
        return FALSE;
    }

    InputMode = OriginalInputMode;
    InputMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT);

    if (!SetConsoleMode(InputHandle, InputMode))
    {
        return FALSE;
    }

    OutputMode = OriginalOutputMode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    if (!SetConsoleMode(OutputHandle, OutputMode))
    {
        SetConsoleMode(InputHandle, OriginalInputMode);
        return FALSE;
    }

    HaveConsoleMode      = TRUE;
    PendingHighSurrogate = 0;
    return TRUE;
#else
    struct termios Raw;

    fflush(stdout);

    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO))
    {
        return FALSE;
    }

    if (tcgetattr(STDIN_FILENO, &OriginalTermios) != 0)
    {
        return FALSE;
    }

    Raw = OriginalTermios;

    Raw.c_iflag &= ~(BRKINT | ICRNL | INLCR | IGNCR | INPCK | ISTRIP | IXON | PARMRK);
    Raw.c_oflag &= ~(OPOST);
    Raw.c_cflag &= ~(CSIZE | PARENB);
    Raw.c_cflag |= CS8;
    Raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    Raw.c_cc[VMIN]  = 1;
    Raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &Raw) != 0)
    {
        return FALSE;
    }

    HaveOriginalTermios = TRUE;
    PlatformConsoleWrite("\x1b[?2004h", 8);
    return TRUE;
#endif
}

/**
 * @brief Restore the original console mode
 *
 * @return VOID
 */
VOID
PlatformConsoleEnd(VOID)
{
#ifdef _WIN32
    if (InputHandle != INVALID_HANDLE_VALUE &&
        OutputHandle != INVALID_HANDLE_VALUE &&
        HaveConsoleMode)
    {
        SetConsoleMode(InputHandle, OriginalInputMode);
        SetConsoleMode(OutputHandle, OriginalOutputMode);
        HaveConsoleMode = FALSE;
    }

    InputHandle          = INVALID_HANDLE_VALUE;
    OutputHandle         = INVALID_HANDLE_VALUE;
    PendingHighSurrogate = 0;
#else
    if (HaveOriginalTermios)
    {
        PlatformConsoleWrite("\x1b[?2004l", 8);
        tcsetattr(STDIN_FILENO, TCSANOW, &OriginalTermios);
        HaveOriginalTermios = FALSE;
    }

    free(PasteBuffer);
    PasteBuffer         = NULL;
    PasteBufferCapacity = 0;
#endif
}

/**
 * @brief Get the current terminal width in columns
 *
 * @return UINT32 number of columns (80 when it cannot be determined)
 */
UINT32
PlatformConsoleGetColumns(VOID)
{
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO Information;

    if (GetConsoleScreenBufferInfo(OutputHandle, &Information))
    {
        INT Width = Information.srWindow.Right - Information.srWindow.Left + 1;

        if (Width > 0)
        {
            return (UINT32)Width;
        }
    }
#else
    struct winsize Window;
    const CHAR *   Columns;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &Window) == 0 && Window.ws_col > 0)
    {
        return (UINT32)Window.ws_col;
    }

    Columns = getenv("COLUMNS");

    if (Columns != NULL)
    {
        CHAR *        End   = NULL;
        unsigned long Value = strtoul(Columns, &End, 10);

        if (End != Columns && Value > 0 && Value <= 10000)
        {
            return (UINT32)Value;
        }
    }
#endif

    return 80;
}

/**
 * @brief Reset a key event to its default state
 *
 * @param Key key event to reset
 *
 * @return VOID
 */
static VOID
PlatformConsoleResetKey(PLATFORM_CONSOLE_KEY * Key)
{
    memset(Key, 0, sizeof(*Key));
    Key->Type        = PLATFORM_CONSOLE_KEY_NONE;
    Key->RepeatCount = 1;
}

#ifdef _WIN32

/**
 * @brief Encode one Unicode code point as UTF-8 into a key event
 *
 * @param CodePoint code point to encode
 * @param Key destination key event
 *
 * @return VOID
 */
static VOID
PlatformConsoleEncodeUtf8(UINT32 CodePoint, PLATFORM_CONSOLE_KEY * Key)
{
    Key->TextLength = 0;

    if (CodePoint <= 0x7F)
    {
        Key->Text[0]    = (CHAR)CodePoint;
        Key->TextLength = 1;
    }
    else if (CodePoint <= 0x7FF)
    {
        Key->Text[0]    = (CHAR)(0xC0 | (CodePoint >> 6));
        Key->Text[1]    = (CHAR)(0x80 | (CodePoint & 0x3F));
        Key->TextLength = 2;
    }
    else if (CodePoint <= 0xFFFF)
    {
        Key->Text[0]    = (CHAR)(0xE0 | (CodePoint >> 12));
        Key->Text[1]    = (CHAR)(0x80 | ((CodePoint >> 6) & 0x3F));
        Key->Text[2]    = (CHAR)(0x80 | (CodePoint & 0x3F));
        Key->TextLength = 3;
    }
    else
    {
        Key->Text[0]    = (CHAR)(0xF0 | (CodePoint >> 18));
        Key->Text[1]    = (CHAR)(0x80 | ((CodePoint >> 12) & 0x3F));
        Key->Text[2]    = (CHAR)(0x80 | ((CodePoint >> 6) & 0x3F));
        Key->Text[3]    = (CHAR)(0x80 | (CodePoint & 0x3F));
        Key->TextLength = 4;
    }
}

/**
 * @brief Translate a Win32 key event into a console key action
 *
 * @param Event Win32 key event
 * @param Key destination key event
 *
 * @return VOID
 */
static VOID
PlatformConsoleTranslateWindowsKey(const KEY_EVENT_RECORD * Event, PLATFORM_CONSOLE_KEY * Key)
{
    const BOOLEAN ControlPressed = (Event->dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
    const BOOLEAN AltPressed     = (Event->dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0;
    WCHAR         Character      = Event->uChar.UnicodeChar;

    switch (Event->wVirtualKeyCode)
    {
    case VK_UP:
        Key->Type = PLATFORM_CONSOLE_KEY_UP;
        return;
    case VK_DOWN:
        Key->Type = PLATFORM_CONSOLE_KEY_DOWN;
        return;
    case VK_LEFT:
        Key->Type = (ControlPressed || AltPressed) ? PLATFORM_CONSOLE_KEY_WORD_LEFT : PLATFORM_CONSOLE_KEY_LEFT;
        return;
    case VK_RIGHT:
        Key->Type = (ControlPressed || AltPressed) ? PLATFORM_CONSOLE_KEY_WORD_RIGHT : PLATFORM_CONSOLE_KEY_RIGHT;
        return;
    case VK_HOME:
        Key->Type = PLATFORM_CONSOLE_KEY_HOME;
        return;
    case VK_END:
        Key->Type = PLATFORM_CONSOLE_KEY_END;
        return;
    case VK_DELETE:
        Key->Type = PLATFORM_CONSOLE_KEY_DELETE;
        return;
    case VK_PRIOR:
        Key->Type = PLATFORM_CONSOLE_KEY_PAGE_UP;
        return;
    case VK_NEXT:
        Key->Type = PLATFORM_CONSOLE_KEY_PAGE_DOWN;
        return;
    case VK_RETURN:
        Key->Type = PLATFORM_CONSOLE_KEY_ENTER;
        return;
    case VK_BACK:
        Key->Type = (ControlPressed || AltPressed) ? PLATFORM_CONSOLE_KEY_KILL_WORD : PLATFORM_CONSOLE_KEY_BACKSPACE;
        return;
    case VK_ESCAPE:
    case VK_TAB:
    case VK_INSERT:
    case VK_PROCESSKEY:
        Key->Type = PLATFORM_CONSOLE_KEY_NONE;
        return;
    default:
        break;
    }

    if (Character == 0)
    {
        Key->Type = PLATFORM_CONSOLE_KEY_NONE;
        return;
    }

    if (ControlPressed && !AltPressed)
    {
        switch (Character)
        {
        case 0x01:
            Key->Type = PLATFORM_CONSOLE_KEY_HOME;
            return;
        case 0x02:
            Key->Type = PLATFORM_CONSOLE_KEY_LEFT;
            return;
        case 0x03:
            Key->Type = PLATFORM_CONSOLE_KEY_CANCEL;
            return;
        case 0x04:
            Key->Type = PLATFORM_CONSOLE_KEY_CTRL_D;
            return;
        case 0x05:
            Key->Type = PLATFORM_CONSOLE_KEY_END;
            return;
        case 0x06:
            Key->Type = PLATFORM_CONSOLE_KEY_RIGHT;
            return;
        case 0x07:
            Key->Type = PLATFORM_CONSOLE_KEY_ABORT;
            return;
        case 0x08:
            Key->Type = PLATFORM_CONSOLE_KEY_BACKSPACE;
            return;
        case 0x09:
            Key->Type = PLATFORM_CONSOLE_KEY_NONE;
            return;
        case 0x0A:
        case 0x0D:
            Key->Type = PLATFORM_CONSOLE_KEY_ENTER;
            return;
        case 0x0B:
            Key->Type = PLATFORM_CONSOLE_KEY_KILL_TO_END;
            return;
        case 0x0C:
            Key->Type = PLATFORM_CONSOLE_KEY_CLEAR_SCREEN;
            return;
        case 0x0E:
            Key->Type = PLATFORM_CONSOLE_KEY_DOWN;
            return;
        case 0x10:
            Key->Type = PLATFORM_CONSOLE_KEY_UP;
            return;
        case 0x12:
            Key->Type = PLATFORM_CONSOLE_KEY_REVERSE_SEARCH;
            return;
        case 0x14:
            Key->Type = PLATFORM_CONSOLE_KEY_TRANSPOSE;
            return;
        case 0x15:
            Key->Type = PLATFORM_CONSOLE_KEY_KILL_TO_START;
            return;
        case 0x17:
            Key->Type = PLATFORM_CONSOLE_KEY_KILL_WORD;
            return;
        case 0x19:
            Key->Type = PLATFORM_CONSOLE_KEY_YANK;
            return;
        case 0x1A:
            Key->Type = PLATFORM_CONSOLE_KEY_CTRL_Z;
            return;
        case 0x7F:
            Key->Type = PLATFORM_CONSOLE_KEY_BACKSPACE;
            return;
        default:
            Key->Type = PLATFORM_CONSOLE_KEY_NONE;
            return;
        }
    }

    if (AltPressed && !ControlPressed)
    {
        switch (Character)
        {
        case L'b':
        case L'B':
            Key->Type = PLATFORM_CONSOLE_KEY_WORD_LEFT;
            return;
        case L'f':
        case L'F':
            Key->Type = PLATFORM_CONSOLE_KEY_WORD_RIGHT;
            return;
        case L'd':
        case L'D':
            Key->Type = PLATFORM_CONSOLE_KEY_KILL_WORD_FORWARD;
            return;
        default:
            Key->Type = PLATFORM_CONSOLE_KEY_NONE;
            return;
        }
    }

    if (Character == L'\r' || Character == L'\n')
    {
        Key->Type = PLATFORM_CONSOLE_KEY_ENTER;
        return;
    }

    if (Character == L'\b' || Character == 0x7F)
    {
        Key->Type = PLATFORM_CONSOLE_KEY_BACKSPACE;
        return;
    }

    if (Character < 0x20)
    {
        Key->Type = PLATFORM_CONSOLE_KEY_NONE;
        return;
    }

    if (Character >= 0xD800 && Character <= 0xDBFF)
    {
        PendingHighSurrogate = Character;
        Key->Type            = PLATFORM_CONSOLE_KEY_NONE;
        return;
    }

    if (Character >= 0xDC00 && Character <= 0xDFFF)
    {
        if (PendingHighSurrogate != 0)
        {
            UINT32 CodePoint = 0x10000 +
                               (((UINT32)PendingHighSurrogate - 0xD800) << 10) +
                               ((UINT32)Character - 0xDC00);

            PendingHighSurrogate = 0;
            Key->Type            = PLATFORM_CONSOLE_KEY_CHARACTER;

            if (Event->wRepeatCount > 1)
            {
                Key->RepeatCount = Event->wRepeatCount;
            }

            PlatformConsoleEncodeUtf8(CodePoint, Key);
        }
        else
        {
            Key->Type = PLATFORM_CONSOLE_KEY_NONE;
        }

        return;
    }

    PendingHighSurrogate = 0;
    Key->Type            = PLATFORM_CONSOLE_KEY_CHARACTER;

    if (Event->wRepeatCount > 1)
    {
        Key->RepeatCount = Event->wRepeatCount;
    }

    PlatformConsoleEncodeUtf8((UINT32)Character, Key);
}

/**
 * @brief Read one decoded key event
 *
 * @param Key receives the decoded key
 *
 * @return BOOLEAN TRUE on success, FALSE on end-of-file or read error
 */
BOOLEAN
PlatformConsoleReadKey(PLATFORM_CONSOLE_KEY * Key)
{
    for (;;)
    {
        INPUT_RECORD Record;
        DWORD        Count = 0;

        PlatformConsoleResetKey(Key);

        if (!ReadConsoleInputW(InputHandle, &Record, 1, &Count))
        {
            Key->Type = PLATFORM_CONSOLE_KEY_ERROR;
            return FALSE;
        }

        if (Count == 0)
        {
            continue;
        }

        if (Record.EventType == WINDOW_BUFFER_SIZE_EVENT)
        {
            Key->Type = PLATFORM_CONSOLE_KEY_RESIZE;
            return TRUE;
        }

        if (Record.EventType != KEY_EVENT)
        {
            continue;
        }

        if (!Record.Event.KeyEvent.bKeyDown)
        {
            continue;
        }

        PlatformConsoleTranslateWindowsKey(&Record.Event.KeyEvent, Key);
        return TRUE;
    }
}

/**
 * @brief Suspend is not supported on Windows
 *
 * @return BOOLEAN always FALSE
 */
BOOLEAN
PlatformConsoleSuspend(VOID)
{
    return FALSE;
}

#else

typedef enum _PLATFORM_CONSOLE_RAW_READ_RESULT
{
    PLATFORM_CONSOLE_RAW_READ_OK,
    PLATFORM_CONSOLE_RAW_READ_END_OF_FILE,
    PLATFORM_CONSOLE_RAW_READ_ERROR,
    PLATFORM_CONSOLE_RAW_READ_TIMEOUT,
} PLATFORM_CONSOLE_RAW_READ_RESULT;

/**
 * @brief Read one raw byte from stdin
 *
 * @param Byte receives the byte
 *
 * @return PLATFORM_CONSOLE_RAW_READ_RESULT read result
 */
static PLATFORM_CONSOLE_RAW_READ_RESULT
PlatformConsoleReadRawByte(unsigned char * Byte)
{
    for (;;)
    {
        ssize_t Count = read(STDIN_FILENO, Byte, 1);

        if (Count == 1)
        {
            return PLATFORM_CONSOLE_RAW_READ_OK;
        }

        if (Count == 0)
        {
            return PLATFORM_CONSOLE_RAW_READ_END_OF_FILE;
        }

        if (errno == EINTR)
        {
            continue;
        }

        return PLATFORM_CONSOLE_RAW_READ_ERROR;
    }
}

/**
 * @brief Read one raw byte with a timeout
 *
 * @param Byte receives the byte
 * @param TimeoutMs timeout in milliseconds
 *
 * @return PLATFORM_CONSOLE_RAW_READ_RESULT read result
 */
static PLATFORM_CONSOLE_RAW_READ_RESULT
PlatformConsoleReadRawByteWithTimeout(unsigned char * Byte, int TimeoutMs)
{
    struct pollfd Descriptor;
    int           Result;

    Descriptor.fd      = STDIN_FILENO;
    Descriptor.events  = POLLIN;
    Descriptor.revents = 0;

    do
    {
        Result = poll(&Descriptor, 1, TimeoutMs);
    } while (Result < 0 && errno == EINTR);

    if (Result == 0)
    {
        return PLATFORM_CONSOLE_RAW_READ_TIMEOUT;
    }

    if (Result < 0)
    {
        return PLATFORM_CONSOLE_RAW_READ_ERROR;
    }

    return PlatformConsoleReadRawByte(Byte);
}

/**
 * @brief Return the number of bytes in a UTF-8 sequence from its lead byte
 *
 * @param Lead lead byte
 *
 * @return UINT32 sequence length
 */
static UINT32
PlatformConsoleUtf8CharLength(unsigned char Lead)
{
    if (Lead < 0x80)
    {
        return 1;
    }
    if ((Lead & 0xE0) == 0xC0)
    {
        return 2;
    }
    if ((Lead & 0xF0) == 0xE0)
    {
        return 3;
    }
    if ((Lead & 0xF8) == 0xF0)
    {
        return 4;
    }
    return 1;
}

/**
 * @brief Map a control byte to a console key action
 *
 * @param Character control byte
 * @param Key destination key event
 *
 * @return VOID
 */
static VOID
PlatformConsoleMapControl(unsigned char Character, PLATFORM_CONSOLE_KEY * Key)
{
    switch (Character)
    {
    case 0x01:
        Key->Type = PLATFORM_CONSOLE_KEY_HOME;
        break;
    case 0x02:
        Key->Type = PLATFORM_CONSOLE_KEY_LEFT;
        break;
    case 0x03:
        Key->Type = PLATFORM_CONSOLE_KEY_CANCEL;
        break;
    case 0x04:
        Key->Type = PLATFORM_CONSOLE_KEY_CTRL_D;
        break;
    case 0x05:
        Key->Type = PLATFORM_CONSOLE_KEY_END;
        break;
    case 0x06:
        Key->Type = PLATFORM_CONSOLE_KEY_RIGHT;
        break;
    case 0x07:
        Key->Type = PLATFORM_CONSOLE_KEY_ABORT;
        break;
    case 0x0B:
        Key->Type = PLATFORM_CONSOLE_KEY_KILL_TO_END;
        break;
    case 0x0C:
        Key->Type = PLATFORM_CONSOLE_KEY_CLEAR_SCREEN;
        break;
    case 0x0E:
        Key->Type = PLATFORM_CONSOLE_KEY_DOWN;
        break;
    case 0x10:
        Key->Type = PLATFORM_CONSOLE_KEY_UP;
        break;
    case 0x12:
        Key->Type = PLATFORM_CONSOLE_KEY_REVERSE_SEARCH;
        break;
    case 0x14:
        Key->Type = PLATFORM_CONSOLE_KEY_TRANSPOSE;
        break;
    case 0x15:
        Key->Type = PLATFORM_CONSOLE_KEY_KILL_TO_START;
        break;
    case 0x17:
        Key->Type = PLATFORM_CONSOLE_KEY_KILL_WORD;
        break;
    case 0x19:
        Key->Type = PLATFORM_CONSOLE_KEY_YANK;
        break;
    case 0x1A:
        Key->Type = PLATFORM_CONSOLE_KEY_CTRL_Z;
        break;
    default:
        Key->Type = PLATFORM_CONSOLE_KEY_NONE;
        break;
    }
}

/**
 * @brief Ensure the paste buffer can hold the requested size
 *
 * @param Required required capacity in bytes
 *
 * @return BOOLEAN TRUE on success
 */
static BOOLEAN
PlatformConsoleEnsurePasteCapacity(UINT32 Required)
{
    UINT32 Capacity;
    CHAR * NewBuffer;

    if (Required <= PasteBufferCapacity)
    {
        return TRUE;
    }

    Capacity = PasteBufferCapacity == 0 ? 256 : PasteBufferCapacity;

    while (Capacity < Required)
    {
        if (Capacity > PLATFORM_CONSOLE_MAX_PASTE_LENGTH / 2)
        {
            Capacity = PLATFORM_CONSOLE_MAX_PASTE_LENGTH;
            break;
        }

        Capacity *= 2;
    }

    NewBuffer = (CHAR *)realloc(PasteBuffer, Capacity);

    if (NewBuffer == NULL)
    {
        return FALSE;
    }

    PasteBuffer         = NewBuffer;
    PasteBufferCapacity = Capacity;
    return TRUE;
}

/**
 * @brief Read a bracketed paste sequence into the platform paste buffer
 *
 * @param Key destination key event
 *
 * @return VOID
 */
static VOID
PlatformConsoleReadBracketedPaste(PLATFORM_CONSOLE_KEY * Key)
{
    static const CHAR Terminator[] = "\x1b[201~";

    CHAR          Tail[sizeof(Terminator) - 1];
    UINT32        TailLength = 0;
    UINT32        Pasted     = 0;
    BOOLEAN       Failed     = FALSE;
    unsigned char Character;

    while (PlatformConsoleReadRawByte(&Character) == PLATFORM_CONSOLE_RAW_READ_OK)
    {
        if (Pasted < PLATFORM_CONSOLE_MAX_PASTE_LENGTH)
        {
            if (!PlatformConsoleEnsurePasteCapacity(Pasted + 1))
            {
                Failed = TRUE;
            }
            else
            {
                PasteBuffer[Pasted++] = (CHAR)Character;
            }
        }

        if (TailLength < sizeof(Tail))
        {
            Tail[TailLength++] = (CHAR)Character;
        }
        else
        {
            memmove(Tail, Tail + 1, sizeof(Tail) - 1);
            Tail[sizeof(Tail) - 1] = (CHAR)Character;
        }

        if (TailLength == sizeof(Tail) &&
            memcmp(Tail, Terminator, sizeof(Tail)) == 0)
        {
            if (Pasted >= sizeof(Tail) &&
                memcmp(PasteBuffer + Pasted - sizeof(Tail), Terminator, sizeof(Tail)) == 0)
            {
                Pasted -= (UINT32)sizeof(Tail);
            }

            break;
        }
    }

    if (Failed)
    {
        Key->Type = PLATFORM_CONSOLE_KEY_ERROR;
        return;
    }

    Key->Type        = PLATFORM_CONSOLE_KEY_PASTE;
    Key->PasteData   = Pasted > 0 ? PasteBuffer : NULL;
    Key->PasteLength = Pasted;
}

/**
 * @brief Map an ANSI CSI/SS3 sequence to a console key action
 *
 * @param Sequence escape sequence, starting with '[' or 'O'
 * @param Key destination key event
 *
 * @return VOID
 */
static VOID
PlatformConsoleMapCsiSequence(const CHAR * Sequence, UINT32 Length, PLATFORM_CONSOLE_KEY * Key)
{
    CHAR         Final;
    BOOLEAN      WordModifier;
    int          Code;
    const CHAR * Cursor;

    Key->Type = PLATFORM_CONSOLE_KEY_NONE;

    if (Length < 2)
    {
        return;
    }

    if (Sequence[0] == 'O')
    {
        switch (Sequence[1])
        {
        case 'A':
            Key->Type = PLATFORM_CONSOLE_KEY_UP;
            break;
        case 'B':
            Key->Type = PLATFORM_CONSOLE_KEY_DOWN;
            break;
        case 'C':
            Key->Type = PLATFORM_CONSOLE_KEY_RIGHT;
            break;
        case 'D':
            Key->Type = PLATFORM_CONSOLE_KEY_LEFT;
            break;
        case 'H':
            Key->Type = PLATFORM_CONSOLE_KEY_HOME;
            break;
        case 'F':
            Key->Type = PLATFORM_CONSOLE_KEY_END;
            break;
        default:
            break;
        }

        return;
    }

    Final = Sequence[Length - 1];

    if (Length == 2)
    {
        switch (Final)
        {
        case 'A':
            Key->Type = PLATFORM_CONSOLE_KEY_UP;
            break;
        case 'B':
            Key->Type = PLATFORM_CONSOLE_KEY_DOWN;
            break;
        case 'C':
            Key->Type = PLATFORM_CONSOLE_KEY_RIGHT;
            break;
        case 'D':
            Key->Type = PLATFORM_CONSOLE_KEY_LEFT;
            break;
        case 'H':
            Key->Type = PLATFORM_CONSOLE_KEY_HOME;
            break;
        case 'F':
            Key->Type = PLATFORM_CONSOLE_KEY_END;
            break;
        default:
            break;
        }

        return;
    }

    if (Final == '~')
    {
        Code = atoi(Sequence + 1);

        switch (Code)
        {
        case 1:
        case 7:
            Key->Type = PLATFORM_CONSOLE_KEY_HOME;
            break;
        case 2:
            Key->Type = PLATFORM_CONSOLE_KEY_NONE;
            break;
        case 3:
            Key->Type = PLATFORM_CONSOLE_KEY_DELETE;
            break;
        case 4:
        case 8:
            Key->Type = PLATFORM_CONSOLE_KEY_END;
            break;
        case 5:
            Key->Type = PLATFORM_CONSOLE_KEY_PAGE_UP;
            break;
        case 6:
            Key->Type = PLATFORM_CONSOLE_KEY_PAGE_DOWN;
            break;
        default:
            break;
        }

        return;
    }

    WordModifier = FALSE;
    Cursor       = strstr(Sequence, ";3");

    if (Cursor == NULL)
    {
        Cursor = strstr(Sequence, ";4");
    }

    if (Cursor == NULL)
    {
        Cursor = strstr(Sequence, ";5");
    }

    if (Cursor == NULL)
    {
        Cursor = strstr(Sequence, ";6");
    }

    if (Cursor != NULL)
    {
        WordModifier = TRUE;
    }

    switch (Final)
    {
    case 'A':
        Key->Type = PLATFORM_CONSOLE_KEY_UP;
        break;
    case 'B':
        Key->Type = PLATFORM_CONSOLE_KEY_DOWN;
        break;
    case 'C':
        Key->Type = WordModifier ? PLATFORM_CONSOLE_KEY_WORD_RIGHT : PLATFORM_CONSOLE_KEY_RIGHT;
        break;
    case 'D':
        Key->Type = WordModifier ? PLATFORM_CONSOLE_KEY_WORD_LEFT : PLATFORM_CONSOLE_KEY_LEFT;
        break;
    case 'H':
        Key->Type = PLATFORM_CONSOLE_KEY_HOME;
        break;
    case 'F':
        Key->Type = PLATFORM_CONSOLE_KEY_END;
        break;
    default:
        Key->Type = PLATFORM_CONSOLE_KEY_NONE;
        break;
    }
}

/**
 * @brief Read and decode an escape sequence
 *
 * @param Key destination key event
 *
 * @return VOID
 */
static VOID
PlatformConsoleReadEscapeSequence(PLATFORM_CONSOLE_KEY * Key)
{
    unsigned char                    First;
    PLATFORM_CONSOLE_RAW_READ_RESULT Result;
    CHAR                             Sequence[32];
    UINT32                           Length = 1;
    unsigned char                    Character;

    Result = PlatformConsoleReadRawByteWithTimeout(&First, PLATFORM_CONSOLE_ESCAPE_TIMEOUT_MS);

    if (Result != PLATFORM_CONSOLE_RAW_READ_OK)
    {
        Key->Type = PLATFORM_CONSOLE_KEY_NONE;
        return;
    }

    if (First == '[' || First == 'O')
    {
        Sequence[0] = (CHAR)First;

        while (Length < sizeof(Sequence) - 1)
        {
            if (PlatformConsoleReadRawByteWithTimeout(&Character, PLATFORM_CONSOLE_ESCAPE_TIMEOUT_MS) != PLATFORM_CONSOLE_RAW_READ_OK)
            {
                break;
            }

            Sequence[Length++] = (CHAR)Character;

            if (Character >= 0x40 && Character <= 0x7E)
            {
                break;
            }
        }

        Sequence[Length] = '\0';

        if (Length == 5 && memcmp(Sequence, "[200~", 5) == 0)
        {
            PlatformConsoleReadBracketedPaste(Key);
            return;
        }

        PlatformConsoleMapCsiSequence(Sequence, Length, Key);
        return;
    }

    if (First == 'b' || First == 'B')
    {
        Key->Type = PLATFORM_CONSOLE_KEY_WORD_LEFT;
        return;
    }

    if (First == 'f' || First == 'F')
    {
        Key->Type = PLATFORM_CONSOLE_KEY_WORD_RIGHT;
        return;
    }

    if (First == 'd' || First == 'D')
    {
        Key->Type = PLATFORM_CONSOLE_KEY_KILL_WORD_FORWARD;
        return;
    }

    if (First == 0x7F || First == 0x08)
    {
        Key->Type = PLATFORM_CONSOLE_KEY_KILL_WORD;
        return;
    }

    Key->Type = PLATFORM_CONSOLE_KEY_NONE;
}

/**
 * @brief Read one decoded key event
 *
 * @param Key receives the decoded key
 *
 * @return BOOLEAN TRUE on success, FALSE on end-of-file or read error
 */
BOOLEAN
PlatformConsoleReadKey(PLATFORM_CONSOLE_KEY * Key)
{
    unsigned char                    Character;
    PLATFORM_CONSOLE_RAW_READ_RESULT Result;
    UINT32                           Expected;
    UINT32                           Index;

    PlatformConsoleResetKey(Key);

    Result = PlatformConsoleReadRawByte(&Character);

    if (Result == PLATFORM_CONSOLE_RAW_READ_END_OF_FILE)
    {
        Key->Type = PLATFORM_CONSOLE_KEY_END_OF_FILE;
        return FALSE;
    }

    if (Result == PLATFORM_CONSOLE_RAW_READ_ERROR)
    {
        Key->Type = PLATFORM_CONSOLE_KEY_ERROR;
        return FALSE;
    }

    if (Character == 0x1B)
    {
        PlatformConsoleReadEscapeSequence(Key);
        return TRUE;
    }

    if (Character == 0x7F || Character == 0x08)
    {
        Key->Type = PLATFORM_CONSOLE_KEY_BACKSPACE;
        return TRUE;
    }

    if (Character == '\r' || Character == '\n')
    {
        Key->Type = PLATFORM_CONSOLE_KEY_ENTER;
        return TRUE;
    }

    if (Character < 0x20)
    {
        PlatformConsoleMapControl(Character, Key);
        return TRUE;
    }

    if (Character < 0x80)
    {
        Key->Type       = PLATFORM_CONSOLE_KEY_CHARACTER;
        Key->Text[0]    = (CHAR)Character;
        Key->TextLength = 1;
        return TRUE;
    }

    Expected        = PlatformConsoleUtf8CharLength(Character);
    Key->Text[0]    = (CHAR)Character;
    Key->TextLength = 1;

    for (Index = 1; Index < Expected; Index++)
    {
        unsigned char Continuation;

        if (PlatformConsoleReadRawByteWithTimeout(&Continuation, PLATFORM_CONSOLE_UTF8_TIMEOUT_MS) != PLATFORM_CONSOLE_RAW_READ_OK)
        {
            break;
        }

        if ((Continuation & 0xC0) != 0x80)
        {
            break;
        }

        Key->Text[Index] = (CHAR)Continuation;
        Key->TextLength++;
    }

    Key->Type = PLATFORM_CONSOLE_KEY_CHARACTER;
    return TRUE;
}

/**
 * @brief Suspend the process and resume console mode afterwards
 *
 * @return BOOLEAN TRUE if console mode was restored after resume
 */
BOOLEAN
PlatformConsoleSuspend(VOID)
{
    PlatformConsoleEnd();
    raise(SIGTSTP);
    return PlatformConsoleBegin();
}

#endif
