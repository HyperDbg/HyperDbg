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
 *          - SetupPathForFileName: build the absolute path of a file that sits
 *            next to the running executable. The Windows version uses
 *            GetModuleFileName + strip-after-last-'\\' + append '\\'+FileName +
 *            optional existence check. The Linux equivalent is:
 *              readlink("/proc/self/exe", ...)  (the kernel-provided symlink to
 *              the current process's own binary, i.e. the GetModuleFileName
 *              counterpart), then strrchr(..., '/') to strip the program name,
 *              append '/' + FileName, and (if CheckFileExists) verify with
 *              access(path, F_OK). Mind BufferLength bounds on every write.
 *            This is a generic "find a file beside my binary" helper (also used
 *            for the hwdbg test/script files, not just the driver), so it is the
 *            one that is actually worth implementing for real later.
 *
 * @version 0.1
 * @date 2026-07-18
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

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
 * @return BOOLEAN FALSE — not implemented on Linux yet. The real version should
 *         use readlink("/proc/self/exe") + strip + append + access(); see the
 *         file header TODO(Linux).
 */
BOOLEAN
SetupPathForFileName(const CHAR *                                  FileName,
                     _Inout_updates_bytes_all_(BufferLength) PCHAR FileLocation,
                     ULONG                                         BufferLength,
                     BOOLEAN                                       CheckFileExists)
{
    UNREFERENCED_PARAMETER(FileName);
    UNREFERENCED_PARAMETER(FileLocation);
    UNREFERENCED_PARAMETER(BufferLength);
    UNREFERENCED_PARAMETER(CheckFileExists);

    ShowMessages("err, SetupPathForFileName is not supported on Linux yet\n");
    return FALSE;
}

#endif // __linux__
