/**
 * @file line-editor.cpp
 * @author scriptk1d
 * @brief Interactive command line editor with history support
 * @details
 *          This is a small, dependency-free line editor for the HyperDbg CLI.
 *          It gives the CLI the same basic editing behaviour users expect from
 *          gdb/readline:
 *
 *          - Up/Down and Ctrl+P/Ctrl+N navigate the command history
 *          - Left/Right, Home/End and Ctrl+A/Ctrl+E move the cursor
 *          - Backspace/Delete, Ctrl+K/Ctrl+U/Ctrl+W/Ctrl+Y edit the line
 *          - Ctrl+R starts reverse incremental history search
 *          - Ctrl+C cancels the current line
 *          - Ctrl+D on an empty line (or EOF from a pipe) exits the prompt
 *
 *          The editor only takes over the terminal when stdin and stdout are
 *          both TTYs; otherwise it falls back to std::getline() so scripts and
 *          pipelines keep working. Terminal state is always restored before a
 *          command is executed, which keeps HyperDbg's existing CTRL+C
 *          (BreakController) handling intact while the debuggee is running.
 *
 * @version 0.1
 * @date 2026-11-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "line-editor.h"

#include <cctype>
#include <cstdint>
#include <deque>
#include <fstream>
#include <iostream>
#include <system_error>

//
// The platform headers are included after the STL headers above.
// platform-console.h pulls in the SDK on Linux, whose __in/__out macros must
// not be visible while libstdc++'s <memory> internals are parsed.
//
#include "platform/user/header/platform-file.h"
#include "platform/user/header/platform-console.h"

namespace {
constexpr std::size_t DEFAULT_HISTORY_LIMIT       = 1000;
constexpr std::size_t MAXIMUM_HISTORY_LINE_LENGTH = 8 * 1024;
constexpr std::size_t MAXIMUM_HISTORY_FILE_SIZE   = 8 * 1024 * 1024;
constexpr std::size_t MAXIMUM_COMMAND_LENGTH      = 1024 * 1024;

/**
 * @brief Return the number of bytes in a UTF-8 sequence from its lead byte
 */
std::size_t
Utf8CharLength(unsigned char Lead)
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
 * @brief Decode one UTF-8 code point, falling back to one byte on invalid input
 */
std::uint32_t
DecodeUtf8(const std::string & Text, std::size_t Position, std::size_t & Length)
{
    Length = 0;

    if (Position >= Text.size())
    {
        return 0;
    }

    const unsigned char * Bytes     = reinterpret_cast<const unsigned char *>(Text.data());
    std::size_t           Available = Text.size() - Position;
    unsigned char         Lead      = Bytes[Position];

    if (Lead < 0x80)
    {
        Length = 1;
        return Lead;
    }

    std::size_t Expected = Utf8CharLength(Lead);

    if (Expected == 1 || Expected > Available)
    {
        Length = 1;
        return Lead;
    }

    for (std::size_t i = 1; i < Expected; i++)
    {
        if ((Bytes[Position + i] & 0xC0) != 0x80)
        {
            Length = 1;
            return Lead;
        }
    }

    std::uint32_t CodePoint = 0;

    switch (Expected)
    {
    case 2:
        CodePoint = ((Lead & 0x1F) << 6) | (Bytes[Position + 1] & 0x3F);
        break;
    case 3:
        CodePoint = ((Lead & 0x0F) << 12) |
                    ((Bytes[Position + 1] & 0x3F) << 6) |
                    (Bytes[Position + 2] & 0x3F);
        break;
    case 4:
        CodePoint = ((Lead & 0x07) << 18) |
                    ((Bytes[Position + 1] & 0x3F) << 12) |
                    ((Bytes[Position + 2] & 0x3F) << 6) |
                    (Bytes[Position + 3] & 0x3F);
        break;
    default:
        CodePoint = Lead;
        break;
    }

    Length = Expected;
    return CodePoint;
}

/**
 * @brief Return true for zero-width combining marks
 */
bool
IsCombiningMark(std::uint32_t CodePoint)
{
    return (CodePoint >= 0x0300 && CodePoint <= 0x036F) ||
           (CodePoint >= 0x1AB0 && CodePoint <= 0x1AFF) ||
           (CodePoint >= 0x1DC0 && CodePoint <= 0x1DFF) ||
           (CodePoint >= 0x20D0 && CodePoint <= 0x20FF) ||
           (CodePoint >= 0xFE20 && CodePoint <= 0xFE2F);
}

/**
 * @brief Display width of one code point (sufficient for terminal layout)
 */
std::size_t
CodePointWidth(std::uint32_t CodePoint)
{
    if (CodePoint == 0)
    {
        return 0;
    }
    if (CodePoint < 0x20 || (CodePoint >= 0x7F && CodePoint < 0xA0))
    {
        return 0;
    }
    if (IsCombiningMark(CodePoint) ||
        CodePoint == 0x200B ||
        CodePoint == 0x200C ||
        CodePoint == 0x200D ||
        (CodePoint >= 0x2060 && CodePoint <= 0x206F) ||
        CodePoint == 0xFEFF)
    {
        return 0;
    }

    //
    // East Asian wide/fullwidth ranges and emoji. This is intentionally a
    // heuristic; it only needs to keep cursor positioning sane.
    //
    if ((CodePoint >= 0x1100 && CodePoint <= 0x115F) ||
        (CodePoint >= 0x2E80 && CodePoint <= 0x303E) ||
        (CodePoint >= 0x3041 && CodePoint <= 0x33FF) ||
        (CodePoint >= 0x3400 && CodePoint <= 0x4DBF) ||
        (CodePoint >= 0x4E00 && CodePoint <= 0x9FFF) ||
        (CodePoint >= 0xA000 && CodePoint <= 0xA4CF) ||
        (CodePoint >= 0xAC00 && CodePoint <= 0xD7A3) ||
        (CodePoint >= 0xF900 && CodePoint <= 0xFAFF) ||
        (CodePoint >= 0xFE10 && CodePoint <= 0xFE19) ||
        (CodePoint >= 0xFE30 && CodePoint <= 0xFE6F) ||
        (CodePoint >= 0xFF00 && CodePoint <= 0xFF60) ||
        (CodePoint >= 0xFFE0 && CodePoint <= 0xFFE6) ||
        (CodePoint >= 0x1F300 && CodePoint <= 0x1F64F) ||
        (CodePoint >= 0x1F900 && CodePoint <= 0x1F9FF) ||
        (CodePoint >= 0x20000 && CodePoint <= 0x3FFFD))
    {
        return 2;
    }

    return 1;
}

/**
 * @brief Compute the display column reached after rendering Text[Begin, End)
 */
std::size_t
DisplayColumn(const std::string & Text, std::size_t Begin, std::size_t End, std::size_t Column)
{
    std::size_t Position = Begin;

    while (Position < End)
    {
        std::size_t   Length;
        std::uint32_t CodePoint = DecodeUtf8(Text, Position, Length);

        if (Length == 0)
        {
            break;
        }

        if (CodePoint == '\t')
        {
            Column += 8 - (Column % 8);
        }
        else
        {
            Column += CodePointWidth(CodePoint);
        }

        Position += Length;
    }

    return Column;
}

/**
 * @brief Byte index of the UTF-8 code point before Position
 */
std::size_t
PreviousUtf8Position(const std::string & Text, std::size_t Position)
{
    if (Position == 0)
    {
        return 0;
    }

    std::size_t Index   = Position - 1;
    std::size_t Scanned = 0;

    while (Index > 0 &&
           Scanned < 3 &&
           (static_cast<unsigned char>(Text[Index]) & 0xC0) == 0x80)
    {
        Index--;
        Scanned++;
    }

    return Index;
}

/**
 * @brief Byte index of the UTF-8 code point after Position
 */
std::size_t
NextUtf8Position(const std::string & Text, std::size_t Position)
{
    if (Position >= Text.size())
    {
        return Text.size();
    }

    std::size_t Length = Utf8CharLength(static_cast<unsigned char>(Text[Position]));

    if (Length > Text.size() - Position)
    {
        Length = Text.size() - Position;
    }

    return Position + Length;
}

/**
 * @brief True for bytes that belong to a word (ASCII alnum, '_', non-ASCII)
 */
bool
IsWordByte(unsigned char Byte)
{
    return std::isalnum(Byte) != 0 || Byte == '_' || Byte >= 0x80;
}

/**
 * @brief Move to the beginning of the previous word
 */
std::size_t
WordLeftPosition(const std::string & Text, std::size_t Position)
{
    while (Position > 0)
    {
        std::size_t Previous = PreviousUtf8Position(Text, Position);

        if (IsWordByte(static_cast<unsigned char>(Text[Previous])))
        {
            break;
        }

        Position = Previous;
    }

    while (Position > 0)
    {
        std::size_t Previous = PreviousUtf8Position(Text, Position);

        if (!IsWordByte(static_cast<unsigned char>(Text[Previous])))
        {
            break;
        }

        Position = Previous;
    }

    return Position;
}

/**
 * @brief Move to the end of the next word
 */
std::size_t
WordRightPosition(const std::string & Text, std::size_t Position)
{
    while (Position < Text.size() &&
           !IsWordByte(static_cast<unsigned char>(Text[Position])))
    {
        Position = NextUtf8Position(Text, Position);
    }

    while (Position < Text.size() &&
           IsWordByte(static_cast<unsigned char>(Text[Position])))
    {
        Position = NextUtf8Position(Text, Position);
    }

    return Position;
}

/**
 * @brief Force input into one printable physical line
 *
 * @details Control bytes (including CR/LF/TAB and ESC) are replaced with
 *          spaces. This keeps pasted text from injecting terminal escape
 *          sequences and guarantees that a history entry is exactly one line.
 */
std::string
SanitizeSingleLine(const std::string & Text)
{
    std::string Result;
    Result.reserve(Text.size());

    std::size_t Position = 0;

    while (Position < Text.size())
    {
        unsigned char Byte = static_cast<unsigned char>(Text[Position]);

        if (Byte < 0x20 || Byte == 0x7F)
        {
            Result.push_back(' ');
            Position++;
            continue;
        }

        if (Byte < 0x80)
        {
            Result.push_back(Text[Position]);
            Position++;
            continue;
        }

        std::size_t   Length;
        std::uint32_t CodePoint = DecodeUtf8(Text, Position, Length);

        if (Length == 1)
        {
            //
            // Invalid UTF-8: keep the raw byte; the terminal will render a
            // replacement glyph and the command parser will see the byte.
            //
            Result.push_back(Text[Position]);
            Position++;
            continue;
        }

        //
        // C1 controls include the single-byte CSI/OSC equivalents in some
        // terminals (e.g. U+009B). Never pass them through to the terminal.
        //
        if (CodePoint >= 0x80 && CodePoint <= 0x9F)
        {
            Result.push_back(' ');
        }
        else
        {
            Result.append(Text, Position, Length);
        }

        Position += Length;
    }

    return Result;
}

/**
 * @brief Append a platform-appropriate line ending to a byte buffer
 *
 * @param Text destination buffer
 *
 * @return VOID
 */
void
AppendNewLine(std::string & Text)
{
    Text += PLATFORM_CONSOLE_NEWLINE;
}

/**
 * @brief Convert a decoded character event to UTF-8 text, honoring repeats
 *
 * @param Key decoded key event
 *
 * @return std::string character text
 */
std::string
KeyText(const PLATFORM_CONSOLE_KEY & Key)
{
    std::string Text(Key.Text, Key.TextLength);

    if (Key.RepeatCount <= 1 || Text.empty())
    {
        return Text;
    }

    std::string Repeated;
    Repeated.reserve(Text.size() * Key.RepeatCount);

    for (UINT32 Index = 0; Index < Key.RepeatCount; Index++)
    {
        Repeated += Text;
    }

    return Repeated;
}

/**
 * @brief Convert a paste event to text
 *
 * @param Key decoded key event
 *
 * @return std::string pasted text
 */
std::string
PasteText(const PLATFORM_CONSOLE_KEY & Key)
{
    if (Key.PasteData == nullptr || Key.PasteLength == 0)
    {
        return {};
    }

    return std::string(Key.PasteData, Key.PasteLength);
}

/**
 * @brief ASCII-only lower-casing used by the history search
 */
std::string
ToLowerAscii(const std::string & Text)
{
    std::string Result = Text;

    for (char & Character : Result)
    {
        if (Character >= 'A' && Character <= 'Z')
        {
            Character = static_cast<char>(Character - 'A' + 'a');
        }
    }

    return Result;
}

} // namespace

namespace hyperdbg {

/**
 * @brief Private implementation of the line editor
 */
class LineEditor::Impl
{
public:
    Impl() = default;

    ~Impl()
    {
        PlatformConsoleEnd();

        if (!HistoryPath.empty())
        {
            try
            {
                SaveHistory(HistoryPath);
            }
            catch (...)
            {
                //
                // Destructors must not throw; losing history is not fatal.
                //
            }
        }
    }

    void SetHistoryPath(const std::filesystem::path & Path)
    {
        HistoryPath = Path;
    }

    void SetHistoryLimit(std::size_t Limit)
    {
        HistoryLimit = Limit;

        while (History.size() > HistoryLimit)
        {
            History.pop_front();
        }

        if (HistoryIndex > History.size())
        {
            HistoryIndex = History.size();
        }
    }

    void ClearHistory()
    {
        History.clear();
        HistoryIndex = 0;
        HistoryDraft.clear();
    }

    void AddHistory(const std::string & Line)
    {
        if (Line.empty() || Line.size() > MAXIMUM_HISTORY_LINE_LENGTH)
        {
            HistoryIndex = History.size();
            return;
        }

        std::string Sanitized = SanitizeSingleLine(Line);

        if (!History.empty() && History.back() == Sanitized)
        {
            HistoryIndex = History.size();
            return;
        }

        History.push_back(Sanitized);

        while (History.size() > HistoryLimit)
        {
            History.pop_front();
        }

        HistoryIndex = History.size();
        HistoryDraft.clear();
    }

    bool LoadHistory(const std::filesystem::path & Path)
    {
        ClearHistory();

        if (Path.empty())
        {
            return false;
        }

        std::error_code Error;

        if (!std::filesystem::is_regular_file(Path, Error))
        {
            return false;
        }

        std::uintmax_t Size = std::filesystem::file_size(Path, Error);

        if (Error || Size > MAXIMUM_HISTORY_FILE_SIZE)
        {
            return false;
        }

        std::ifstream File(Path, std::ios::binary);

        if (!File)
        {
            return false;
        }

        std::string Line;
        bool        FirstLine = true;

        while (std::getline(File, Line))
        {
            if (!Line.empty() && Line.back() == '\r')
            {
                Line.pop_back();
            }

            if (FirstLine)
            {
                FirstLine = false;

                if (Line.size() >= 3 &&
                    static_cast<unsigned char>(Line[0]) == 0xEF &&
                    static_cast<unsigned char>(Line[1]) == 0xBB &&
                    static_cast<unsigned char>(Line[2]) == 0xBF)
                {
                    Line.erase(0, 3);
                }
            }

            if (Line.empty() || Line.size() > MAXIMUM_HISTORY_LINE_LENGTH)
            {
                continue;
            }

            Line = SanitizeSingleLine(Line);

            if (!History.empty() && History.back() == Line)
            {
                continue;
            }

            History.push_back(Line);

            while (History.size() > HistoryLimit)
            {
                History.pop_front();
            }
        }

        HistoryIndex = History.size();
        return true;
    }

    bool SaveHistory(const std::filesystem::path & Path) const
    {
        if (Path.empty())
        {
            return false;
        }

        std::error_code Error;

        std::filesystem::path Directory = Path.parent_path();

        if (!Directory.empty())
        {
            std::filesystem::create_directories(Directory, Error);

            if (Error)
            {
                return false;
            }
        }

        std::filesystem::path Temporary = Path;
        Temporary += ".tmp.";
        Temporary += std::to_string(PlatformConsoleGetProcessId());

        {
            std::ofstream File(Temporary, std::ios::binary | std::ios::trunc);

            if (!File)
            {
                return false;
            }

            for (const std::string & Line : History)
            {
                std::string Sanitized = SanitizeSingleLine(Line);

                File.write(Sanitized.data(), static_cast<std::streamsize>(Sanitized.size()));
                File.put('\n');
            }

            File.flush();

            if (!File)
            {
                File.close();
                std::filesystem::remove(Temporary, Error);
                return false;
            }
        }

        //
        // History may contain paths or other sensitive text; keep it private.
        //
        std::error_code PermissionError;

        std::filesystem::permissions(Temporary,
                                     std::filesystem::perms::owner_read |
                                         std::filesystem::perms::owner_write,
                                     std::filesystem::perm_options::replace,
                                     PermissionError);

        if (!PlatformReplaceFile(Temporary, Path))
        {
            std::error_code Ignore;
            std::filesystem::remove(Temporary, Ignore);
            return false;
        }

        return true;
    }

    LINE_EDITOR_RESULT ReadLine(const std::string & Prompt, std::string & Line)
    {
        Line.clear();

        if (!PlatformConsoleIsInteractive())
        {
            return ReadLineFallback(Prompt, Line);
        }

        if (!PlatformConsoleBegin())
        {
            return ReadLineFallback(Prompt, Line);
        }

        ResetEditState(Prompt);
        Render();

        LINE_EDITOR_RESULT Result = LINE_EDITOR_RESULT::LINE_EDITOR_SUCCESS;

        for (;;)
        {
            PLATFORM_CONSOLE_KEY Key;

            if (!PlatformConsoleReadKey(&Key))
            {
                Result = (Key.Type == PLATFORM_CONSOLE_KEY_END_OF_FILE) ? LINE_EDITOR_RESULT::LINE_EDITOR_END_OF_FILE : LINE_EDITOR_RESULT::LINE_EDITOR_ERROR;
                FinishLine();
                break;
            }

            if (SearchActive && HandleSearchKey(Key))
            {
                continue;
            }

            switch (Key.Type)
            {
            case PLATFORM_CONSOLE_KEY_CHARACTER:
                InsertText(KeyText(Key));
                break;

            case PLATFORM_CONSOLE_KEY_PASTE:
                InsertText(PasteText(Key));
                break;

            case PLATFORM_CONSOLE_KEY_ENTER:
            {
                bool AtEnd = (Cursor == Buffer.size());

                Cursor = Buffer.size();

                if (!Rendered || !AtEnd)
                {
                    Render();
                }

                FinishLine();
                PlatformConsoleEnd();
                Line = Buffer;
                AddHistory(Line);
                return LINE_EDITOR_RESULT::LINE_EDITOR_SUCCESS;
            }

            case PLATFORM_CONSOLE_KEY_BACKSPACE:
                Backspace();
                break;

            case PLATFORM_CONSOLE_KEY_DELETE:
                Delete();
                break;

            case PLATFORM_CONSOLE_KEY_LEFT:
                MoveCursorLeft();
                break;

            case PLATFORM_CONSOLE_KEY_RIGHT:
                MoveCursorRight();
                break;

            case PLATFORM_CONSOLE_KEY_WORD_LEFT:
                MoveCursorWordLeft();
                break;

            case PLATFORM_CONSOLE_KEY_WORD_RIGHT:
                MoveCursorWordRight();
                break;

            case PLATFORM_CONSOLE_KEY_HOME:
                MoveCursorHome();
                break;

            case PLATFORM_CONSOLE_KEY_END:
                MoveCursorEnd();
                break;

            case PLATFORM_CONSOLE_KEY_UP:
            case PLATFORM_CONSOLE_KEY_PAGE_UP:
                PreviousHistory();
                break;

            case PLATFORM_CONSOLE_KEY_DOWN:
            case PLATFORM_CONSOLE_KEY_PAGE_DOWN:
                NextHistory();
                break;

            case PLATFORM_CONSOLE_KEY_KILL_TO_END:
                KillToEnd();
                break;

            case PLATFORM_CONSOLE_KEY_KILL_TO_START:
                KillToStart();
                break;

            case PLATFORM_CONSOLE_KEY_KILL_WORD:
                KillWord();
                break;

            case PLATFORM_CONSOLE_KEY_KILL_WORD_FORWARD:
                KillWordForward();
                break;

            case PLATFORM_CONSOLE_KEY_YANK:
                Yank();
                break;

            case PLATFORM_CONSOLE_KEY_TRANSPOSE:
                Transpose();
                break;

            case PLATFORM_CONSOLE_KEY_CLEAR_SCREEN:
                ClearScreen();
                break;

            case PLATFORM_CONSOLE_KEY_REVERSE_SEARCH:
                StartSearch();
                break;

            case PLATFORM_CONSOLE_KEY_ABORT:
                Beep();
                break;

            case PLATFORM_CONSOLE_KEY_RESIZE:
                Render();
                break;

            case PLATFORM_CONSOLE_KEY_CTRL_D:
                if (Buffer.empty())
                {
                    Result = LINE_EDITOR_RESULT::LINE_EDITOR_END_OF_FILE;
                    FinishLine();
                    PlatformConsoleEnd();
                    return Result;
                }

                Delete();
                break;

            case PLATFORM_CONSOLE_KEY_CTRL_Z:
                if (PLATFORM_CONSOLE_SUPPORTS_SUSPEND)
                {
                    //
                    // Suspend like a normal shell foreground job. The
                    // platform layer restores the terminal before stopping
                    // and re-enters it after fg/bg.
                    //
                    Rendered     = false;
                    OldCursorRow = 0;
                    FinishLine();

                    if (!PlatformConsoleSuspend())
                    {
                        return LINE_EDITOR_RESULT::LINE_EDITOR_ERROR;
                    }

                    Render();
                    break;
                }

                if (!Buffer.empty())
                {
                    Beep();
                    break;
                }

                Result = LINE_EDITOR_RESULT::LINE_EDITOR_END_OF_FILE;
                FinishLine();
                PlatformConsoleEnd();
                return Result;

            case PLATFORM_CONSOLE_KEY_CANCEL:
            {
                std::string Output;
                AppendClearRenderedLine(Output);
                Output += "^C";
                AppendNewLine(Output);
                Write(Output);
                PlatformConsoleEnd();
                return LINE_EDITOR_RESULT::LINE_EDITOR_CANCELLED;
            }

            case PLATFORM_CONSOLE_KEY_END_OF_FILE:
                FinishLine();
                PlatformConsoleEnd();
                return LINE_EDITOR_RESULT::LINE_EDITOR_END_OF_FILE;

            case PLATFORM_CONSOLE_KEY_ERROR:
                FinishLine();
                PlatformConsoleEnd();
                return LINE_EDITOR_RESULT::LINE_EDITOR_ERROR;

            case PLATFORM_CONSOLE_KEY_NONE:
                break;
            }
        }

        PlatformConsoleEnd();
        return Result;
    }

private:
    /**
     * @brief Reset per-line editing state
     */
    void ResetEditState(const std::string & Prompt)
    {
        PromptText  = Prompt;
        PromptWidth = DisplayColumn(PromptText, 0, PromptText.size(), 0);
        Buffer.clear();
        Cursor          = 0;
        Rendered        = false;
        OldCursorRow    = 0;
        RenderedColumns = 0;
        CursorOnNewLine = false;
        KillBuffer.clear();
        HistoryIndex = History.size();
        HistoryDraft.clear();
        SearchActive = false;
        SearchQuery.clear();
        SearchIndex = History.size();
        SearchSavedBuffer.clear();
        SearchSavedCursor = 0;
        SearchSavedPrompt.clear();
    }

    /**
     * @brief Render the prompt and edit buffer, preserving the cursor position
     */
    void Render()
    {
        std::size_t Columns = PlatformConsoleGetColumns();

        if (Columns == 0)
        {
            Columns = 80;
        }

        std::string Output;

        CursorOnNewLine = false;

        if (Rendered && RenderedColumns != 0 && RenderedColumns != Columns)
        {
            //
            // The terminal was resized while a wrapped line was on screen.
            // Starting a fresh line is the only portable way to avoid mixing
            // old and new wrapping layouts.
            //
            AppendNewLine(Output);
            Rendered = false;
        }

        std::size_t CursorColumn = DisplayColumn(Buffer, 0, Cursor, PromptWidth);
        std::size_t EndColumn    = DisplayColumn(Buffer, 0, Buffer.size(), PromptWidth);

        if (Rendered)
        {
            if (OldCursorRow > 1)
            {
                Output += "\x1b[";
                Output += std::to_string(OldCursorRow - 1);
                Output += "A";
            }

            Output += "\r\x1b[J";
        }

        Output += PromptText;
        Output += Buffer;

        //
        // A cursor sitting exactly on a row boundary needs a real cell on the
        // next row; otherwise terminal "pending wrap" state makes cursor
        // positioning ambiguous.
        //
        std::size_t Rows = (EndColumn + Columns - 1) / Columns;

        if (Rows == 0)
        {
            Rows = 1;
        }

        if (Cursor == Buffer.size() &&
            EndColumn > 0 &&
            (EndColumn % Columns) == 0)
        {
            AppendNewLine(Output);
            Rows++;
            CursorOnNewLine = true;
        }

        std::size_t CursorRow = (CursorColumn / Columns) + 1;

        if (CursorRow > Rows)
        {
            CursorRow = Rows;
        }

        if (Rows > CursorRow)
        {
            Output += "\x1b[";
            Output += std::to_string(Rows - CursorRow);
            Output += "A";
        }

        std::size_t CursorColumnOnRow = CursorColumn % Columns;

        if (CursorColumnOnRow > 0)
        {
            Output += "\r\x1b[";
            Output += std::to_string(CursorColumnOnRow);
            Output += "C";
        }
        else
        {
            Output += '\r';
        }

        Write(Output);

        OldCursorRow    = CursorRow;
        Rendered        = true;
        RenderedColumns = Columns;
    }

    /**
     * @brief Append the escape sequence that erases the current edit buffer
     */
    void AppendClearRenderedLine(std::string & Output)
    {
        if (Rendered)
        {
            if (OldCursorRow > 1)
            {
                Output += "\x1b[";
                Output += std::to_string(OldCursorRow - 1);
                Output += "A";
            }

            Output += '\r';
        }
        else
        {
            Output += '\r';
        }

        Output += "\x1b[J";
        Rendered        = false;
        OldCursorRow    = 0;
        CursorOnNewLine = false;
    }

    /**
     * @brief Insert text at the cursor
     */
    void InsertText(const std::string & Text)
    {
        std::string Sanitized = SanitizeSingleLine(Text);

        if (Sanitized.empty())
        {
            return;
        }

        if (Buffer.size() + Sanitized.size() > MAXIMUM_COMMAND_LENGTH)
        {
            Beep();
            return;
        }

        bool AtEnd = (Cursor == Buffer.size());

        Buffer.insert(Cursor, Sanitized);
        Cursor += Sanitized.size();

        //
        // Avoid a full prompt redraw for the common case of typing at the end
        // of a line that still fits on one screen row. This keeps normal typing
        // cheap while all other operations use the general renderer.
        //
        if (AtEnd && Rendered && !SearchActive)
        {
            std::size_t Columns = PlatformConsoleGetColumns();

            if (Columns == 0)
            {
                Columns = 80;
            }

            std::size_t EndColumn = DisplayColumn(Buffer, 0, Buffer.size(), PromptWidth);

            if (OldCursorRow == 1 && EndColumn < Columns)
            {
                Write(Sanitized);
                return;
            }
        }

        Render();
    }

    /**
     * @brief Delete the code point before the cursor
     */
    void Backspace()
    {
        if (Cursor == 0)
        {
            Beep();
            return;
        }

        std::size_t Previous = PreviousUtf8Position(Buffer, Cursor);
        Buffer.erase(Previous, Cursor - Previous);
        Cursor = Previous;
        Render();
    }

    /**
     * @brief Delete the code point at the cursor
     */
    void Delete()
    {
        if (Cursor >= Buffer.size())
        {
            Beep();
            return;
        }

        std::size_t Next = NextUtf8Position(Buffer, Cursor);
        Buffer.erase(Cursor, Next - Cursor);
        Render();
    }

    void MoveCursorLeft()
    {
        if (Cursor == 0)
        {
            Beep();
            return;
        }

        Cursor = PreviousUtf8Position(Buffer, Cursor);
        Render();
    }

    void MoveCursorRight()
    {
        if (Cursor >= Buffer.size())
        {
            Beep();
            return;
        }

        Cursor = NextUtf8Position(Buffer, Cursor);
        Render();
    }

    void MoveCursorWordLeft()
    {
        std::size_t NewCursor = WordLeftPosition(Buffer, Cursor);

        if (NewCursor == Cursor)
        {
            Beep();
            return;
        }

        Cursor = NewCursor;
        Render();
    }

    void MoveCursorWordRight()
    {
        std::size_t NewCursor = WordRightPosition(Buffer, Cursor);

        if (NewCursor == Cursor)
        {
            Beep();
            return;
        }

        Cursor = NewCursor;
        Render();
    }

    void MoveCursorHome()
    {
        if (Cursor == 0)
        {
            return;
        }

        Cursor = 0;
        Render();
    }

    void MoveCursorEnd()
    {
        if (Cursor == Buffer.size())
        {
            return;
        }

        Cursor = Buffer.size();
        Render();
    }

    void KillToEnd()
    {
        if (Cursor >= Buffer.size())
        {
            Beep();
            return;
        }

        KillBuffer = Buffer.substr(Cursor);
        Buffer.erase(Cursor);
        Render();
    }

    void KillToStart()
    {
        if (Cursor == 0)
        {
            Beep();
            return;
        }

        KillBuffer = Buffer.substr(0, Cursor);
        Buffer.erase(0, Cursor);
        Cursor = 0;
        Render();
    }

    void KillWord()
    {
        if (Cursor == 0)
        {
            Beep();
            return;
        }

        std::size_t Start = WordLeftPosition(Buffer, Cursor);
        KillBuffer        = Buffer.substr(Start, Cursor - Start);
        Buffer.erase(Start, Cursor - Start);
        Cursor = Start;
        Render();
    }

    void KillWordForward()
    {
        if (Cursor >= Buffer.size())
        {
            Beep();
            return;
        }

        std::size_t End = WordRightPosition(Buffer, Cursor);
        KillBuffer      = Buffer.substr(Cursor, End - Cursor);
        Buffer.erase(Cursor, End - Cursor);
        Render();
    }

    void Yank()
    {
        if (KillBuffer.empty())
        {
            Beep();
            return;
        }

        InsertText(KillBuffer);
    }

    void Transpose()
    {
        if (Buffer.empty() || Cursor == 0)
        {
            Beep();
            return;
        }

        if (Cursor == Buffer.size())
        {
            std::size_t Last     = PreviousUtf8Position(Buffer, Cursor);
            std::size_t Previous = PreviousUtf8Position(Buffer, Last);

            if (Last == Previous)
            {
                Beep();
                return;
            }

            std::string Before = Buffer.substr(Previous, Last - Previous);
            std::string LastCh = Buffer.substr(Last);

            Buffer.replace(Previous, Buffer.size() - Previous, LastCh + Before);
            Cursor = Buffer.size();
            Render();
            return;
        }

        std::size_t Left  = PreviousUtf8Position(Buffer, Cursor);
        std::size_t Right = NextUtf8Position(Buffer, Cursor);

        std::string Before = Buffer.substr(Left, Cursor - Left);
        std::string After  = Buffer.substr(Cursor, Right - Cursor);

        Buffer.replace(Left, Right - Left, After + Before);
        Cursor = Right;
        Render();
    }

    void ClearScreen()
    {
        Write("\x1b[H\x1b[2J");
        Rendered     = false;
        OldCursorRow = 0;
        Render();
    }

    /**
     * @brief Move to the previous history entry
     */
    void PreviousHistory()
    {
        if (History.empty() || HistoryIndex == 0)
        {
            Beep();
            return;
        }

        if (HistoryIndex > History.size())
        {
            HistoryIndex = History.size();
        }

        if (HistoryIndex == History.size())
        {
            HistoryDraft = Buffer;
        }

        HistoryIndex--;
        Buffer = History[HistoryIndex];
        Cursor = Buffer.size();
        Render();
    }

    /**
     * @brief Move to the next history entry (or back to the draft)
     */
    void NextHistory()
    {
        if (HistoryIndex >= History.size())
        {
            Beep();
            return;
        }

        HistoryIndex++;

        if (HistoryIndex == History.size())
        {
            Buffer = HistoryDraft;
        }
        else
        {
            Buffer = History[HistoryIndex];
        }

        Cursor = Buffer.size();
        Render();
    }

    /**
     * @brief Start (or continue) reverse incremental history search
     */
    void StartSearch()
    {
        if (SearchActive)
        {
            SearchOlder();
            return;
        }

        SearchActive      = true;
        SearchSavedBuffer = Buffer;
        SearchSavedCursor = Cursor;
        SearchSavedPrompt = PromptText;
        SearchQuery.clear();
        SearchIndex = History.size();
        UpdateSearchDisplay();
    }

    /**
     * @brief Handle a key while reverse search is active
     *
     * @return true if the key was consumed by the search
     */
    bool HandleSearchKey(const PLATFORM_CONSOLE_KEY & Key)
    {
        switch (Key.Type)
        {
        case PLATFORM_CONSOLE_KEY_CHARACTER:
            SearchQuery += SanitizeSingleLine(KeyText(Key));
            SearchUpdate(false);
            return true;

        case PLATFORM_CONSOLE_KEY_PASTE:
            SearchQuery += SanitizeSingleLine(PasteText(Key));
            SearchUpdate(false);
            return true;

        case PLATFORM_CONSOLE_KEY_BACKSPACE:
            if (!SearchQuery.empty())
            {
                SearchQuery.erase(PreviousUtf8Position(SearchQuery, SearchQuery.size()));
                SearchUpdate(false);
            }
            return true;

        case PLATFORM_CONSOLE_KEY_REVERSE_SEARCH:
            SearchOlder();
            return true;

        case PLATFORM_CONSOLE_KEY_ABORT:
            EndSearch(false, true);
            return true;

        case PLATFORM_CONSOLE_KEY_CLEAR_SCREEN:
            ClearScreen();
            return true;

        case PLATFORM_CONSOLE_KEY_RESIZE:
            Render();
            return true;

        case PLATFORM_CONSOLE_KEY_ENTER:
        case PLATFORM_CONSOLE_KEY_CTRL_D:
        case PLATFORM_CONSOLE_KEY_CTRL_Z:
        case PLATFORM_CONSOLE_KEY_END_OF_FILE:
        case PLATFORM_CONSOLE_KEY_ERROR:
            EndSearch(true, true);
            return false;

        case PLATFORM_CONSOLE_KEY_CANCEL:
            EndSearch(true, false);
            return false;

        default:
            //
            // Any other editing/navigation key ends the search but keeps the
            // matched line, matching readline's behaviour.
            //
            EndSearch(true, true);
            return false;
        }
    }

    /**
     * @brief Re-run the incremental search for the current query
     *
     * @param FromCurrentMatch true to search before the current match, false
     *                         to restart from the newest history entry
     */
    void SearchUpdate(bool FromCurrentMatch)
    {
        if (SearchQuery.empty())
        {
            Buffer      = SearchSavedBuffer;
            Cursor      = SearchSavedCursor;
            SearchIndex = History.size();
            UpdateSearchDisplay();
            return;
        }

        std::string Needle = ToLowerAscii(SearchQuery);
        std::size_t Start  = FromCurrentMatch ? SearchIndex : History.size();

        for (std::size_t Index = Start; Index > 0; Index--)
        {
            std::size_t Candidate = Index - 1;

            if (ToLowerAscii(History[Candidate]).find(Needle) != std::string::npos)
            {
                SearchIndex = Candidate;
                Buffer      = History[Candidate];
                Cursor      = Buffer.size();
                UpdateSearchDisplay();
                return;
            }
        }

        Beep();
        SearchIndex = Start;
        UpdateSearchDisplay();
    }

    /**
     * @brief Find the next older match for the current query
     */
    void SearchOlder()
    {
        if (SearchQuery.empty())
        {
            Beep();
            return;
        }

        if (SearchIndex == 0)
        {
            Beep();
            return;
        }

        std::string Needle = ToLowerAscii(SearchQuery);

        for (std::size_t Index = SearchIndex; Index > 0; Index--)
        {
            std::size_t Candidate = Index - 1;

            if (ToLowerAscii(History[Candidate]).find(Needle) != std::string::npos)
            {
                SearchIndex = Candidate;
                Buffer      = History[Candidate];
                Cursor      = Buffer.size();
                UpdateSearchDisplay();
                return;
            }
        }

        Beep();
    }

    /**
     * @brief Refresh the search prompt
     */
    void UpdateSearchDisplay()
    {
        PromptText  = "(reverse-i-search)`" + SearchQuery + "': ";
        PromptWidth = DisplayColumn(PromptText, 0, PromptText.size(), 0);
        Render();
    }

    /**
     * @brief Leave reverse search mode
     *
     * @param KeepMatch keep the matched line instead of restoring the draft
     * @param RenderNow repaint immediately
     */
    void EndSearch(bool KeepMatch, bool RenderNow)
    {
        if (!SearchActive)
        {
            return;
        }

        SearchActive = false;

        if (!KeepMatch)
        {
            Buffer = SearchSavedBuffer;
            Cursor = SearchSavedCursor;
        }

        PromptText  = SearchSavedPrompt;
        PromptWidth = DisplayColumn(PromptText, 0, PromptText.size(), 0);
        SearchQuery.clear();
        SearchIndex  = History.size();
        HistoryIndex = History.size();

        if (RenderNow)
        {
            Render();
        }
    }

    void Beep()
    {
        Write("\a");
    }

    /**
     * @brief Write a platform-appropriate line ending
     *
     * @details POSIX raw mode has OPOST disabled, so CR must be explicit.
     *          Windows stdout stays in text mode and the CRT turns '\n' into
     *          "\r\n"; writing "\r\n" there would produce "\r\r\n".
     */
    void WriteNewLine()
    {
        PlatformConsoleWriteNewLine();
    }

    /**
     * @brief End the current input line without leaving a blank row behind
     *
     * @details Render() may already have moved the cursor to the next line
     *          when the prompt and buffer exactly filled a terminal row.
     */
    void FinishLine()
    {
        if (!CursorOnNewLine)
        {
            WriteNewLine();
        }

        CursorOnNewLine = false;
    }

    /**
     * @brief Write bytes to the terminal and flush immediately
     */
    void Write(const std::string & Text)
    {
        Write(Text.data(), Text.size());
    }

    void Write(const char * Data, std::size_t Length)
    {
        PlatformConsoleWrite(Data, static_cast<SIZE_T>(Length));
    }

    /**
     * @brief Read one line when the terminal is not interactive
     */
    LINE_EDITOR_RESULT ReadLineFallback(const std::string & Prompt, std::string & Line)
    {
        Line.clear();

        Write(Prompt);

        if (!std::getline(std::cin, Line))
        {
            Write("\n");

            if (std::cin.eof())
            {
                return LINE_EDITOR_RESULT::LINE_EDITOR_END_OF_FILE;
            }

            std::cin.clear();
            return LINE_EDITOR_RESULT::LINE_EDITOR_ERROR;
        }

        if (!Line.empty() && Line.back() == '\r')
        {
            Line.pop_back();
        }

        return LINE_EDITOR_RESULT::LINE_EDITOR_SUCCESS;
    }

public:
    std::deque<std::string> History;
    std::filesystem::path   HistoryPath;
    std::size_t             HistoryLimit = DEFAULT_HISTORY_LIMIT;
    std::size_t             HistoryIndex = 0;
    std::string             HistoryDraft;
    std::string             Buffer;
    std::size_t             Cursor = 0;
    std::string             PromptText;
    std::size_t             PromptWidth     = 0;
    std::size_t             OldCursorRow    = 0;
    std::size_t             RenderedColumns = 0;
    bool                    Rendered        = false;
    bool                    CursorOnNewLine = false;
    std::string             KillBuffer;
    bool                    SearchActive = false;
    std::string             SearchQuery;
    std::size_t             SearchIndex = 0;
    std::string             SearchSavedBuffer;
    std::size_t             SearchSavedCursor = 0;
    std::string             SearchSavedPrompt;
};

LineEditor::LineEditor() : ImplPtr(std::make_unique<Impl>())
{
}

LineEditor::~LineEditor() = default;

void
LineEditor::SetHistoryLimit(std::size_t Limit)
{
    ImplPtr->SetHistoryLimit(Limit);
}

void
LineEditor::AddHistory(const std::string & Line)
{
    ImplPtr->AddHistory(Line);
}

void
LineEditor::SetHistoryPath(const std::filesystem::path & Path)
{
    ImplPtr->SetHistoryPath(Path);
}

void
LineEditor::ClearHistory()
{
    ImplPtr->ClearHistory();
}

bool
LineEditor::LoadHistory(const std::filesystem::path & Path)
{
    return ImplPtr->LoadHistory(Path);
}

bool
LineEditor::SaveHistory(const std::filesystem::path & Path) const
{
    return ImplPtr->SaveHistory(Path);
}

LINE_EDITOR_RESULT
LineEditor::ReadLine(const std::string & Prompt, std::string & Line)
{
    return ImplPtr->ReadLine(Prompt, Line);
}

std::filesystem::path
DefaultHistoryFilePath()
{
    return PlatformGetDefaultHistoryFilePath();
}

} // namespace hyperdbg
