/**
 * @file platform-file.h
 * @author scriptk1d
 * @brief User mode cross-platform helpers for filesystem paths and replacement
 * @details This is a C++ platform helper because std::filesystem::path carries
 *          the native path type on each platform. It owns the differences
 *          between Windows wide paths / MoveFileEx replacement and POSIX
 *          byte paths / atomic rename.
 *
 * @version 0.1
 * @date 2026-11-03
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#include <filesystem>

/**
 * @brief Resolve the default HyperDbg CLI history file
 *
 * @details HYPERDBG_HISTORY overrides the path. An empty value, or the
 *          special value "-", disables persistence. Windows uses
 *          %LOCALAPPDATA%\HyperDbg\history with %USERPROFILE% fallback;
 *          Linux uses $XDG_STATE_HOME/hyperdbg/history with
 *          ~/.local/state/hyperdbg/history fallback.
 *
 * @return std::filesystem::path history path, or an empty path when
 *         persistence is unavailable or disabled
 */
std::filesystem::path
PlatformGetDefaultHistoryFilePath();

/**
 * @brief Replace a destination file with a source file
 *
 * @details POSIX uses an atomic rename. Windows retries after removing the
 *          destination when the first rename fails because the destination
 *          already exists.
 *
 * @param Source source path
 * @param Destination destination path
 *
 * @return bool true on success, false otherwise
 */
bool
PlatformReplaceFile(const std::filesystem::path & Source,
                    const std::filesystem::path & Destination);
