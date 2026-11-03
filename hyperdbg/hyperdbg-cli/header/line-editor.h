/**
 * @file line-editor.h
 * @author scriptk1d
 * @brief Interactive command line editor with history support
 * @details
 * @version 0.1
 * @date 2026-11-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>

namespace hyperdbg {
/**
 * @brief Result of reading one line from the terminal
 */
enum class LINE_EDITOR_RESULT
{
    LINE_EDITOR_SUCCESS,
    LINE_EDITOR_CANCELLED,
    LINE_EDITOR_END_OF_FILE,
    LINE_EDITOR_ERROR,
};

/**
 * @brief Platform-independent line editor
 *
 * @details Supports cursor movement, word movement, history navigation
 *          (Up/Down, Ctrl+P/Ctrl+N), reverse incremental search (Ctrl+R),
 *          line editing (Ctrl+A/E/K/U/W, Home/End/Delete) and persistent
 *          history. When stdin/stdout are not a terminal, it transparently
 *          falls back to std::getline().
 */
class LineEditor
{
public:
    /**
     * @brief Construct a line editor
     */
    LineEditor();

    /**
     * @brief Destroy the line editor and persist history
     */
    ~LineEditor();

    LineEditor(const LineEditor &)             = delete;
    LineEditor & operator=(const LineEditor &) = delete;

    /**
     * @brief Set the maximum number of history entries kept in memory
     *
     * @param Limit maximum number of entries
     *
     * @return VOID
     */
    void SetHistoryLimit(std::size_t Limit);

    /**
     * @brief Add a command to the in-memory history
     *
     * @param Line command line to add
     *
     * @return VOID
     */
    void AddHistory(const std::string & Line);

    /**
     * @brief Set the file used to persist history automatically
     *
     * @details The history is saved when the editor is destroyed, so callers
     *          do not need an atexit handler (which matters because some
     *          HyperDbg commands call exit(0) directly).
     *
     * @param Path history file path
     *
     * @return VOID
     */
    void SetHistoryPath(const std::filesystem::path & Path);

    /**
     * @brief Remove all in-memory history entries
     *
     * @return VOID
     */
    void ClearHistory();

    /**
     * @brief Load history from a file (replaces the current history)
     *
     * @param Path history file path
     *
     * @return bool true on success, false otherwise
     */
    bool LoadHistory(const std::filesystem::path & Path);

    /**
     * @brief Atomically save the in-memory history to a file
     *
     * @param Path history file path
     *
     * @return bool true on success, false otherwise
     */
    bool SaveHistory(const std::filesystem::path & Path) const;

    /**
     * @brief Read one line of input
     *
     * @param Prompt prompt to display before the input
     * @param Line receives the line without the trailing newline
     *
     * @return LINE_EDITOR_RESULT the result of the read operation
     */
    LINE_EDITOR_RESULT ReadLine(const std::string & Prompt, std::string & Line);

private:
    class Impl;
    std::unique_ptr<Impl> ImplPtr;
};

/**
 * @brief Resolve the default per-user history file
 *
 * @details HYPERDBG_HISTORY overrides the path. An empty value, or the
 *          special value "-", disables persistence. Linux uses
 *          $XDG_STATE_HOME/hyperdbg/history (falling back to
 *          ~/.local/state/hyperdbg/history); Windows uses
 *          %LOCALAPPDATA%\HyperDbg\history.
 *
 * @return std::filesystem::path history path, or an empty path if persistence
 *         is unavailable
 */
std::filesystem::path
DefaultHistoryFilePath();

} // namespace hyperdbg
