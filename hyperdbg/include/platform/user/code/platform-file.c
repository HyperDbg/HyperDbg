/**
 * @file platform-file.c
 * @author scriptk1d
 * @brief User mode cross-platform filesystem path and replacement helpers
 * @details See platform-file.h. This file is compiled as C++ because
 *          std::filesystem::path is the native path type on both platforms.
 *
 * @version 0.1
 * @date 2026-11-03
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "../header/platform-file.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <system_error>

#ifdef _WIN32

/**
 * @brief Read a Windows environment variable as a wide string
 *
 * @param Name variable name
 * @param Value receives the value; empty when the variable exists but is empty
 *
 * @return bool true when the variable exists, false otherwise
 */
static bool
PlatformGetWindowsEnvironmentString(const wchar_t * Name, std::wstring & Value)
{
    wchar_t *   Raw  = nullptr;
    std::size_t Size = 0;

    if (_wdupenv_s(&Raw, &Size, Name) != 0)
    {
        std::free(Raw);
        return false;
    }

    if (Raw == nullptr)
    {
        return false;
    }

    Value.assign(Raw);
    std::free(Raw);
    return true;
}

/**
 * @brief Resolve the default HyperDbg CLI history file
 *
 * @return std::filesystem::path history path, or an empty path
 */
std::filesystem::path
PlatformGetDefaultHistoryFilePath()
{
    std::wstring Override;

    if (PlatformGetWindowsEnvironmentString(L"HYPERDBG_HISTORY", Override))
    {
        if (Override.empty() || Override == L"-")
        {
            return {};
        }

        return std::filesystem::path(Override);
    }

    std::wstring LocalAppData;

    if (PlatformGetWindowsEnvironmentString(L"LOCALAPPDATA", LocalAppData) &&
        !LocalAppData.empty())
    {
        return std::filesystem::path(LocalAppData) / L"HyperDbg" / L"history";
    }

    std::wstring UserProfile;

    if (PlatformGetWindowsEnvironmentString(L"USERPROFILE", UserProfile) &&
        !UserProfile.empty())
    {
        return std::filesystem::path(UserProfile) / L".hyperdbg_history";
    }

    return {};
}

/**
 * @brief Replace a destination file with a source file
 *
 * @param Source source path
 * @param Destination destination path
 *
 * @return bool true on success, false otherwise
 */
bool
PlatformReplaceFile(const std::filesystem::path & Source,
                    const std::filesystem::path & Destination)
{
    std::error_code Error;

    std::filesystem::rename(Source, Destination, Error);

    if (Error)
    {
        //
        // MoveFileEx does not replace an existing destination on all Windows
        // versions, so retry after removing it.
        //
        std::error_code RemoveError;

        std::filesystem::remove(Destination, RemoveError);
        Error.clear();
        std::filesystem::rename(Source, Destination, Error);
    }

    return !Error;
}

#else

/**
 * @brief Resolve the default HyperDbg CLI history file
 *
 * @return std::filesystem::path history path, or an empty path
 */
std::filesystem::path
PlatformGetDefaultHistoryFilePath()
{
    const char * Override = std::getenv("HYPERDBG_HISTORY");

    if (Override != nullptr)
    {
        if (Override[0] == 0 || std::strcmp(Override, "-") == 0)
        {
            return {};
        }

        return std::filesystem::path(Override);
    }

    const char * XdgStateHome = std::getenv("XDG_STATE_HOME");

    if (XdgStateHome != nullptr && XdgStateHome[0] == '/')
    {
        return std::filesystem::path(XdgStateHome) / "hyperdbg" / "history";
    }

    const char * Home = std::getenv("HOME");

    if (Home != nullptr && Home[0] != 0)
    {
        return std::filesystem::path(Home) / ".local" / "state" / "hyperdbg" / "history";
    }

    return {};
}

/**
 * @brief Replace a destination file with a source file
 *
 * @param Source source path
 * @param Destination destination path
 *
 * @return bool true on success, false otherwise
 */
bool
PlatformReplaceFile(const std::filesystem::path & Source,
                    const std::filesystem::path & Destination)
{
    std::error_code Error;

    std::filesystem::rename(Source, Destination, Error);

    return !Error;
}

#endif
