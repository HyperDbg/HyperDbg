/**
 * @file install-linux.cpp
 * @author Max Raulea (max.raulea@hyperdbg.org)
 * @brief Linux implementations of the driver-loader (install.cpp)
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
 *          SetupPathForFileName is a generic "find a file beside my binary"
 *          helper rather than a driver-loading one (it is also used for the
 *          hwdbg test/script files and the PCI ID database), so it is
 *          implemented for real here: readlink("/proc/self/exe") is the
 *          GetModuleFileName counterpart, and the rest of the Windows logic
 *          (strip the program name, append the requested file, optionally check
 *          that it exists) maps over unchanged.
 *
 *          TODO(Linux) to make the rest real once a Linux kernel component lands:
 *          - ManageDriver: load/unload the future HyperDbg Linux kernel module.
 *            The natural backend is insmod/rmmod semantics via the finit_module(2)
 *            / delete_module(2) syscalls (or libkmod), taking the .ko path built
 *            by SetupPathForFileName. DRIVER_FUNC_INSTALL/START -> load,
 *            DRIVER_FUNC_STOP/REMOVE -> unload. Requires CAP_SYS_MODULE (root).
 *
 * @version 0.1
 * @date 2026-07-18
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#ifdef __linux__

#    include <unistd.h> // readlink()/access() for SetupPathForFileName
#    include <errno.h>  // errno/strerror() for the readlink() failure message

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
 * @param FileName the file to locate (e.g. the driver or a test/script file).
 *        Windows-style '\\' separators are accepted and normalized
 * @param FileLocation out buffer receiving the full path
 * @param BufferLength size of FileLocation in bytes. The executable's own path
 *        is read into the same buffer first, so it has to fit as well
 * @param CheckFileExists whether to verify the resulting path exists
 *
 * @return BOOLEAN whether the path could be built (and, when asked for, whether
 *         the resulting file exists)
 */
BOOLEAN
SetupPathForFileName(const CHAR *                                  FileName,
                     _Inout_updates_bytes_all_(BufferLength) PCHAR FileLocation,
                     ULONG                                         BufferLength,
                     BOOLEAN                                       CheckFileExists)
{
    ssize_t PathLength;
    CHAR *  ProgramName;
    SIZE_T  DirectoryLength;
    SIZE_T  FileNameLength;

    if (FileName == NULL || FileLocation == NULL || BufferLength == 0)
    {
        return FALSE;
    }

    //
    // "/proc/self/exe" is the kernel-provided symlink to the binary of the
    // running process, which makes it the counterpart of the
    // GetModuleFileName(GetModuleHandle(NULL), ...) used on Windows.
    // readlink() never writes a null terminator, so the last byte of the
    // buffer is reserved for it
    //
    PathLength = readlink("/proc/self/exe", FileLocation, BufferLength - 1);

    if (PathLength < 0)
    {
        ShowMessages("err, unable to resolve the path of the current executable (%s)\n",
                     strerror(errno));

        return FALSE;
    }

    //
    // readlink() truncates silently, so a result that fills the buffer means
    // the path is (possibly) incomplete and would point at the wrong file
    //
    if ((ULONG)PathLength >= BufferLength - 1)
    {
        ShowMessages("err, the path of the current executable does not fit in the buffer\n");

        return FALSE;
    }

    FileLocation[PathLength] = '\0';

    //
    // Remove the program name and keep the directory that contains it; this is
    // the '/' counterpart of the strrchr(FileLocation, '\\') in install.cpp
    //
    ProgramName = strrchr(FileLocation, '/');

    if (ProgramName == NULL)
    {
        ShowMessages("err, unable to resolve the directory of the current executable\n");

        return FALSE;
    }

    DirectoryLength = (SIZE_T)(ProgramName - FileLocation);

    //
    // The directory, the separator, the file name and the null terminator all
    // have to fit; this is what the StringCbCat() calls check on Windows
    //
    FileNameLength = strlen(FileName);

    if (DirectoryLength + FileNameLength + 2 > BufferLength)
    {
        ShowMessages("err, the path of the target file does not fit in the buffer\n");

        return FALSE;
    }

    FileLocation[DirectoryLength] = '/';
    memcpy(FileLocation + DirectoryLength + 1, FileName, FileNameLength + 1);

    //
    // The file names come from shared headers and are spelled the Windows way
    // (e.g. "constants\\pci.ids"), so the separators of the appended part are
    // normalized; otherwise the backslashes would end up inside a file name
    //
    for (SIZE_T i = DirectoryLength + 1; i <= DirectoryLength + FileNameLength; i++)
    {
        if (FileLocation[i] == '\\')
        {
            FileLocation[i] = '/';
        }
    }

    if (CheckFileExists && access(FileLocation, F_OK) != 0)
    {
        ShowMessages("err, target file is not loaded\n");

        return FALSE;
    }

    return TRUE;
}

#endif // __linux__
