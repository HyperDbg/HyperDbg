/**
 * @file install-linux.cpp
 * @author Max Raulea (max.raulea@hyperdbg.org)
 * @brief Linux stub implementations of the driver-loader (install.cpp)
 * @details The Windows implementation (install.cpp) loads/unloads the HyperDbg
 *          kernel-mode driver (the .sys file that contains the actual debugging
 *          engine) through the Windows Service Control Manager (SCM):
 *          InstallDriver/StartDriver/StopDriver/RemoveDriver wrap
 *          CreateService/StartService/ControlService/DeleteService, and
 *          ManageDriver orchestrates them for a given DRIVER_FUNC_* action.
 *
 *          None of that exists on Linux: there is no SCM, and there is no
 *          HyperDbg Linux kernel module to load yet. So the whole translation
 *          unit is swapped out on Linux (CMake `if(UNIX)` REMOVE_ITEM
 *          install.cpp + APPEND install-linux.cpp), mirroring the
 *          symbol.cpp -> symbol-linux.cpp / pe-parser.cpp -> pe-parser-linux.cpp
 *          pattern. install.cpp itself is left 100% untouched for the Windows
 *          build. Only the two public entry points that non-Windows callers
 *          reference are provided here (ManageDriver, SetupPathForFileName); the
 *          four SC_HANDLE helpers are guarded out of install.h on Linux and are
 *          never referenced there.
 *
 *          TODO(Linux) to make these real once a Linux kernel component lands:
 *          - ManageDriver: load/unload the future HyperDbg Linux kernel module.
 *            The natural backend is insmod/rmmod semantics via the finit_module(2)
 *            / delete_module(2) syscalls (or libkmod), taking the .ko path built
 *            by SetupPathForFileName. DRIVER_FUNC_INSTALL/START -> load,
 *            DRIVER_FUNC_STOP/REMOVE -> unload. Requires CAP_SYS_MODULE (root).
 *
 *          SetupPathForFileName is implemented for real: it is the generic
 *          "find a file beside my binary" helper (also used for the test-case
 *          and hwdbg script files, not just the driver). It is the counterpart
 *          of the Windows version: readlink("/proc/self/exe") instead of
 *          GetModuleFileName, then strip the program name, append the file name
 *          and optionally check that the result exists.
 *
 * @version 0.1
 * @date 2026-07-18
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#ifdef __linux__
#    include <unistd.h> // readlink() / access()
#    include <limits.h> // PATH_MAX
#    include <errno.h>
#endif

#ifdef __linux__

/**
 * @brief Install / start / stop / remove the HyperDbg kernel driver.
 *
 * @param DriverName
 * @param ServiceName
 * @param Function one of DRIVER_FUNC_INSTALL / STOP / REMOVE
 *
 * @return BOOLEAN FALSE — driver (un)loading is not supported on Linux yet
 *         (no SCM, and no HyperDbg Linux kernel module). See file header for the
 *         finit_module/delete_module plan.
 */
BOOLEAN
ManageDriver(_In_ LPCTSTR DriverName, _In_ LPCTSTR ServiceName, _In_ UINT16 Function)
{
    UNREFERENCED_PARAMETER(DriverName);
    UNREFERENCED_PARAMETER(ServiceName);
    UNREFERENCED_PARAMETER(Function);

    ShowMessages("err, driver (un)loading is not supported on Linux yet\n");
    return FALSE;
}

/**
 * @brief Build the absolute path of a file located next to the running binary.
 *
 * @param FileName the file to locate (e.g. the driver or a test/script file)
 * @param FileLocation out buffer receiving the full path
 * @param BufferLength size of FileLocation in bytes
 * @param CheckFileExists whether to verify the resulting path exists
 *
 * @return BOOLEAN TRUE if the path was built (and exists, when requested)
 */
BOOLEAN
SetupPathForFileName(const CHAR *                                  FileName,
                     _Inout_updates_bytes_all_(BufferLength) PCHAR FileLocation,
                     ULONG                                         BufferLength,
                     BOOLEAN                                       CheckFileExists)
{
    CHAR    ExePath[PATH_MAX] = {0};
    CHAR *  Pos;
    ssize_t ExePathLen;
    size_t  DirLen;
    size_t  FileNameLen;

    if (FileName == NULL || FileLocation == NULL || BufferLength == 0)
    {
        return FALSE;
    }

    //
    // /proc/self/exe is the counterpart of GetModuleFileName(NULL, ...).
    // readlink() does not null-terminate, the buffer is zeroed above and one
    // byte is kept for the terminator
    //
    ExePathLen = readlink("/proc/self/exe", ExePath, sizeof(ExePath) - 1);

    if (ExePathLen <= 0)
    {
        ShowMessages("err, unable to get the path of the running executable (%x)\n", errno);
        return FALSE;
    }

    //
    // Strip the name of the executable
    //
    Pos = strrchr(ExePath, '/');

    if (Pos != NULL)
    {
        *Pos = '\0';
    }

    //
    // Setup path name to the file ("<directory>/<FileName>")
    //
    DirLen      = strlen(ExePath);
    FileNameLen = strlen(FileName);

    if (DirLen + 1 + FileNameLen + 1 > BufferLength)
    {
        return FALSE;
    }

    memcpy(FileLocation, ExePath, DirLen);
    FileLocation[DirLen] = '/';
    memcpy(FileLocation + DirLen + 1, FileName, FileNameLen + 1);

    //
    // The callers are shared with Windows and pass relative paths with
    // backslashes (e.g., "..\\..\\tests\\..."), which is not a separator here
    //
    for (Pos = FileLocation + DirLen + 1; *Pos != '\0'; Pos++)
    {
        if (*Pos == '\\')
        {
            *Pos = '/';
        }
    }

    if (CheckFileExists)
    {
        //
        // ensure file is in the specified directory
        //
        if (access(FileLocation, F_OK) != 0)
        {
            ShowMessages("err, target file is not loaded\n");
            return FALSE;
        }
    }

    return TRUE;
}

#endif // __linux__
