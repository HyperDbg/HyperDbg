/**
 * @file pt-linux.cpp
 * @author Max Raulea (max.raulea@hyperdbg.org)
 * @brief Linux stub implementations of the !pt (Intel Processor Trace) command (pt.cpp)
 * @details The Windows implementation (pt.cpp) drives Intel PT by attaching to a
 *          live process, so it is built almost entirely out of Win32
 *          process/thread management — roughly 27 raw Win32 call sites, with no
 *          #ifdef guards anywhere in the file:
 *            - CreateToolhelp32Snapshot + Process32First/Next and
 *              Thread32First/Next to walk processes and threads by name/pid/tid,
 *            - OpenProcess / OpenThread plus the handle lifetime around them
 *              (CloseHandle x8),
 *            - SetThreadAffinityMask to pin the traced thread to a core,
 *            - CreateEvent / CreateThread / WaitForMultipleObjects for the
 *              background trace thread and its g_PtTraceStopEvent stop signal,
 *            - two DeviceIoControl + GetLastError pairs.
 *
 *          Those last two are simple renames onto the existing Platform*
 *          wrappers, but everything above needs a real decision per call site —
 *          either route it through a new cross-platform wrapper, or guard the
 *          whole enclosing function for Windows and give Linux a stub — because
 *          the Win32 calls are interleaved with the surrounding walk and UI
 *          logic rather than sitting behind a clean boundary. Porting half the
 *          file would leave it in a worse state than leaving it whole, so until
 *          that work is done the entire translation unit is swapped out on Linux
 *          (CMake `if(UNIX)` REMOVE_ITEM pt.cpp + APPEND pt-linux.cpp). This
 *          mirrors how symbol.cpp, pe-parser.cpp, install.cpp and namedpipe.cpp
 *          are handled; pt.cpp itself is left 100% untouched for the Windows
 *          build.
 *
 *          Only the 4 externally visible functions are provided here — CommandPt
 *          and CommandPtHelp (declared in commands.h / help.h, reached from the
 *          command dispatch table) and HyperDbgPerformPtOperation /
 *          HyperDbgPtMmapSendRequest (declared in debugger.h). Everything else
 *          in pt.cpp is helper code reached only through those entry points, so
 *          it simply does not exist in the Linux translation unit.
 *
 *          TODO(Linux) to make these real: the Toolhelp process/thread walks
 *          become /proc enumeration, OpenProcess/OpenThread become pid/tid
 *          handles (or a ptrace attach), SetThreadAffinityMask becomes
 *          sched_setaffinity(2), and the event/thread machinery becomes the
 *          existing Platform* wrappers. Note the underlying IOCTL transport
 *          (platform-ioctl) is itself still a Linux stub, so a working !pt also
 *          depends on the kernel module landing.
 *
 * @version 0.1
 * @date 2026-07-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#ifdef __linux__

/**
 * @brief help of the !pt command
 *
 * @return VOID
 */
VOID
CommandPtHelp()
{
    ShowMessages("!pt : enables, disables and configures Intel Processor Trace (PT).\n\n");
    ShowMessages("err, the !pt command is not supported on Linux yet\n");
}

/**
 * @brief !pt command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandPt(vector<CommandToken> CommandTokens, string Command)
{
    UNREFERENCED_PARAMETER(CommandTokens);
    UNREFERENCED_PARAMETER(Command);

    ShowMessages("err, the !pt command is not supported on Linux yet\n");
}

/**
 * @brief Send an Intel PT operation request to the kernel
 *
 * @param PtRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperDbgPerformPtOperation(HYPERTRACE_PT_OPERATION_PACKETS * PtRequest)
{
    UNREFERENCED_PARAMETER(PtRequest);

    ShowMessages("err, Intel PT operations are not supported on Linux yet\n");

    return FALSE;
}

/**
 * @brief Send an Intel PT trace-buffer mapping request to the kernel
 *
 * @param MmapRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperDbgPtMmapSendRequest(HYPERTRACE_PT_MMAP_PACKETS * MmapRequest)
{
    UNREFERENCED_PARAMETER(MmapRequest);

    ShowMessages("err, Intel PT buffer mapping is not supported on Linux yet\n");

    return FALSE;
}

#endif // __linux__
