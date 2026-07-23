/**
 * @file pe-parser-linux.cpp
 * @author Max Raulea (max.raulea@hyperdbg.org)
 * @brief Linux stub implementations of the PE (Portable Executable) parser
 * @details The Windows implementation (pe-parser.cpp) parses the full PE image
 *          format and depends on the complete set of Windows IMAGE_* headers,
 *          types and constants (IMAGE_DOS_HEADER, IMAGE_NT_HEADERS,
 *          IMAGE_DATA_DIRECTORY, IMAGE_BASE_RELOCATION, ...), which are not
 *          defined on Linux yet.
 *
 *          PE parsing is only needed once Windows-target debugging on Linux is
 *          implemented. Until then these stubs let the library compile and link
 *          on Linux while keeping all call sites intact.
 *
 *          TODO: recreate the Windows PE-image headers for Linux (a portable
 *                IMAGE_* shim) and port pe-parser.cpp when Windows debugging on
 *                Linux is added, then replace these stubs.
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
 * @brief Show section information and dump a PE image.
 *
 * @return BOOLEAN FALSE — PE parsing is not supported on Linux yet.
 */
BOOLEAN
PeShowSectionInformationAndDump(const WCHAR * AddressOfFile,
                                const CHAR *  SectionToShow,
                                BOOLEAN       Is32Bit)
{
    UNREFERENCED_PARAMETER(AddressOfFile);
    UNREFERENCED_PARAMETER(SectionToShow);
    UNREFERENCED_PARAMETER(Is32Bit);

    ShowMessages("err, PE parsing is not supported on Linux yet\n");
    return FALSE;
}

/**
 * @brief Determine whether a PE image is 32-bit or 64-bit.
 *
 * @return BOOLEAN FALSE — PE parsing is not supported on Linux yet.
 */
BOOLEAN
PeIsPE32BitOr64Bit(const WCHAR * AddressOfFile, PBOOLEAN Is32Bit)
{
    UNREFERENCED_PARAMETER(AddressOfFile);

    if (Is32Bit != NULL)
        *Is32Bit = FALSE;

    ShowMessages("err, PE parsing is not supported on Linux yet\n");
    return FALSE;
}

/**
 * @brief Resolve the syscall number of an ntdll function.
 *
 * @return UINT32 0 — PE parsing is not supported on Linux yet.
 */
UINT32
PeGetSyscallNumber(LPCSTR NtFunctionName)
{
    UNREFERENCED_PARAMETER(NtFunctionName);

    ShowMessages("err, PE parsing is not supported on Linux yet\n");
    return 0;
}

#endif // __linux__
