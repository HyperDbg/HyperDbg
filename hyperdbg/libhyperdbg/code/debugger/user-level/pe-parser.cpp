/**
 * @file pe-parser.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Portable Executable parser
 * @details
 * @version 0.1
 * @date 2021-12-26
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Returns a human-readable name for a PE subsystem identifier
 *
 * Maps the Subsystem field from IMAGE_OPTIONAL_HEADER to a descriptive string.
 * Unknown subsystem values return "Unknown".
 *
 * @param Subsystem The Subsystem value from the PE optional header
 *
 * @return const CHAR* A static string describing the subsystem
 */
static const CHAR *
PeGetSubsystemName(WORD Subsystem)
{
    switch (Subsystem)
    {
    case IMAGE_SUBSYSTEM_NATIVE:
        return "Device Driver(Native windows Process)";
    case IMAGE_SUBSYSTEM_WINDOWS_GUI:
        return "Windows GUI";
    case IMAGE_SUBSYSTEM_WINDOWS_CUI:
        return "Windows CLI";
    case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:
        return "Windows CE GUI";
    default:
        return "Unknown";
    }
}

/**
 * @brief Prints the set DLL characteristics flags from a PE optional header
 *
 * Iterates a table of known IMAGE_DLLCHARACTERISTICS_* flag values and prints
 * the name of each flag that is set in DllCharacteristics, separated by commas.
 * Prints "None" when no flags are set.
 *
 * @param DllCharacteristics The DllCharacteristics field from the PE optional header
 */
static VOID
PeShowDllCharacteristics(WORD DllCharacteristics)
{
    BOOLEAN AnyFlag = FALSE;

    struct DLL_CHARACTERISTIC_NAME
    {
        WORD         Flag;
        const CHAR * Name;
    };

    static const DLL_CHARACTERISTIC_NAME CommonDllCharacteristics[] = {
        {IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA, "High Entropy VA"},
        {IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE, "Dynamic Base"},
        {IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY, "Force Integrity"},
        {IMAGE_DLLCHARACTERISTICS_NX_COMPAT, "NX Compatible"},
        {IMAGE_DLLCHARACTERISTICS_NO_ISOLATION, "No Isolation"},
        {IMAGE_DLLCHARACTERISTICS_NO_SEH, "No SEH"},
        {IMAGE_DLLCHARACTERISTICS_NO_BIND, "No Bind"},
        {IMAGE_DLLCHARACTERISTICS_APPCONTAINER, "AppContainer"},
        {IMAGE_DLLCHARACTERISTICS_WDM_DRIVER, "WDM Driver"},
        {IMAGE_DLLCHARACTERISTICS_GUARD_CF, "Guard CF"},
        {IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE, "Terminal Server Aware"},
    };

    for (UINT32 Index = 0; Index < RTL_NUMBER_OF(CommonDllCharacteristics); Index++)
    {
        if ((DllCharacteristics & CommonDllCharacteristics[Index].Flag) == CommonDllCharacteristics[Index].Flag)
        {
            ShowMessages("%s%s", AnyFlag ? ", " : "", CommonDllCharacteristics[Index].Name);
            AnyFlag = TRUE;
        }
    }

    if (!AnyFlag)
    {
        ShowMessages("None");
    }
}

/**
 * @brief Prints the raw file offset corresponding to the PE entrypoint RVA
 *
 * Resolves AddressOfEntryPoint through the section table to obtain a file offset
 * and prints it. Prints "not mapped" when the RVA cannot be resolved.
 *
 * @param Reader               Pointer to an initialized PE_IMAGE_READER
 * @param AddressOfEntryPoint  The AddressOfEntryPoint field from the PE optional header
 */
static VOID
PeShowEntrypointFileOffset(PPE_IMAGE_READER Reader, DWORD AddressOfEntryPoint)
{
    SIZE_T FileOffset = 0;

    if (PeImageReaderRvaToFileOffset(Reader, AddressOfEntryPoint, 1, &FileOffset))
    {
        ShowMessages("\n%-36s%#llx", "Entrypoint file offset :", (UINT64)FileOffset);
    }
    else
    {
        ShowMessages("\n%-36s%s", "Entrypoint file offset :", "not mapped");
    }
}

/**
 * @brief Returns the name of a PE data directory by its index
 *
 * Maps IMAGE_DIRECTORY_ENTRY_* index values to descriptive strings using
 * the standard ordering defined in the PE specification. Returns "Unknown"
 * for indices at or beyond IMAGE_NUMBEROF_DIRECTORY_ENTRIES.
 *
 * @param Index Zero-based data directory index (IMAGE_DIRECTORY_ENTRY_*)
 *
 * @return const CHAR* A static string naming the data directory
 */
static const CHAR *
PeGetDataDirectoryName(UINT32 Index)
{
    static const CHAR * DirectoryNames[IMAGE_NUMBEROF_DIRECTORY_ENTRIES] = {
        "Export Table",
        "Import Table",
        "Resource Table",
        "Exception Table",
        "Certificate Table",
        "Base Relocation Table",
        "Debug",
        "Architecture",
        "Global Ptr",
        "TLS Table",
        "Load Config Table",
        "Bound Import",
        "Import Address Table",
        "Delay Import Descriptor",
        "CLR Runtime Header",
        "Reserved",
    };

    if (Index < RTL_NUMBER_OF(DirectoryNames))
    {
        return DirectoryNames[Index];
    }

    return "Unknown";
}

/**
 * @brief Adds two DWORD values with overflow detection
 *
 * Returns FALSE without modifying Result when the addition would exceed MAXDWORD;
 * otherwise writes the sum to *Result and returns TRUE.
 *
 * @param Left   First operand
 * @param Right  Second operand
 * @param Result Output pointer that receives the sum on success; must not be NULL
 *
 * @return BOOLEAN TRUE if the addition succeeded, FALSE on overflow or NULL pointer
 */
static BOOLEAN
PeAddDword(DWORD Left, DWORD Right, DWORD * Result)
{
    if (Result == NULL || Right > MAXDWORD - Left)
    {
        return FALSE;
    }

    *Result = Left + Right;
    return TRUE;
}

/**
 * @brief Adds two ULONGLONG values with overflow detection
 *
 * Returns FALSE without modifying Result when the addition would overflow a
 * 64-bit unsigned integer; otherwise writes the sum to *Result and returns TRUE.
 *
 * @param Left   First operand
 * @param Right  Second operand
 * @param Result Output pointer that receives the sum on success; must not be NULL
 *
 * @return BOOLEAN TRUE if the addition succeeded, FALSE on overflow or NULL pointer
 */
static BOOLEAN
PeAddUlonglong(ULONGLONG Left, ULONGLONG Right, ULONGLONG * Result)
{
    if (Result == NULL || Right > (~((ULONGLONG)0) - Left))
    {
        return FALSE;
    }

    *Result = Left + Right;
    return TRUE;
}

/**
 * @brief Comparator for sorting PE_RAW_SECTION_RANGE entries by ascending start offset
 *
 * Intended for use with qsort(). Entries with the same Start are further ordered
 * by End in ascending order.
 *
 * @param Left  Pointer to the first PE_RAW_SECTION_RANGE to compare
 * @param Right Pointer to the second PE_RAW_SECTION_RANGE to compare
 *
 * @return INT Negative if Left < Right, positive if Left > Right, zero if equal
 */
static INT
PeCompareRawSectionRange(const VOID * Left, const VOID * Right)
{
    const PE_RAW_SECTION_RANGE * LeftRange  = (const PE_RAW_SECTION_RANGE *)Left;
    const PE_RAW_SECTION_RANGE * RightRange = (const PE_RAW_SECTION_RANGE *)Right;

    if (LeftRange->Start < RightRange->Start)
    {
        return -1;
    }

    if (LeftRange->Start > RightRange->Start)
    {
        return 1;
    }

    if (LeftRange->End < RightRange->End)
    {
        return -1;
    }

    if (LeftRange->End > RightRange->End)
    {
        return 1;
    }

    return 0;
}

/**
 * @brief Checks whether an RVA range is fully contained within another RVA range
 *
 * Both ranges are expressed as a base RVA and a size. Returns FALSE if any
 * addition overflows or if [Rva, Rva+Size) extends outside [RangeRva, RangeRva+RangeSize).
 *
 * @param RangeRva  Base RVA of the enclosing range
 * @param RangeSize Size of the enclosing range in bytes
 * @param Rva       Base RVA of the range to test
 * @param Size      Size of the range to test in bytes
 *
 * @return BOOLEAN TRUE if [Rva, Rva+Size) lies entirely within [RangeRva, RangeRva+RangeSize)
 */
static BOOLEAN
PeRvaContainsRange(DWORD RangeRva, DWORD RangeSize, DWORD Rva, DWORD Size)
{
    DWORD RangeEnd = 0;
    DWORD RvaEnd   = 0;

    if (!PeAddDword(RangeRva, RangeSize, &RangeEnd) || !PeAddDword(Rva, Size, &RvaEnd))
    {
        return FALSE;
    }

    return Rva >= RangeRva && RvaEnd <= RangeEnd;
}

/**
 * @brief Returns a validated pointer into the image at the location mapped by an RVA
 *
 * Translates Rva to a raw file offset via PeImageReaderRvaToFileOffset and then
 * validates the range through PeImageReaderGetPointerAtOffset.
 *
 * @param Reader  Pointer to an initialized PE_IMAGE_READER
 * @param Rva     Relative virtual address to look up
 * @param Length  Number of bytes that must be accessible at the resolved offset
 * @param Pointer Output pointer set to the resolved image location on success; must not be NULL
 *
 * @return BOOLEAN TRUE on success, FALSE if the RVA cannot be resolved or is out of bounds
 */
static BOOLEAN
PeGetPointerAtRva(PPE_IMAGE_READER Reader, DWORD Rva, DWORD Length, const BYTE ** Pointer)
{
    SIZE_T FileOffset = 0;

    if (!PeImageReaderRvaToFileOffset(Reader, Rva, Length, &FileOffset))
    {
        return FALSE;
    }

    return PeImageReaderGetPointerAtOffset(Reader, FileOffset, Length, Pointer);
}

typedef enum _PE_ASCII_STRING_STATUS
{
    PeAsciiStringInvalid,
    PeAsciiStringOk,
    PeAsciiStringTruncated,
} PE_ASCII_STRING_STATUS;

/**
 * @brief Reads an ASCII string from the image at the given RVA
 *
 * Reads up to MaxLength bytes starting at Rva, stopping at the first null
 * terminator. Non-printable bytes are replaced with '.'. The output buffer is
 * always null-terminated. Returns PeAsciiStringInvalid if any mapped byte
 * cannot be resolved, PeAsciiStringOk when the null terminator is found within
 * MaxLength, and PeAsciiStringTruncated otherwise.
 *
 * @param Reader     Pointer to an initialized PE_IMAGE_READER
 * @param Rva        Starting RVA of the string
 * @param MaxLength  Maximum number of bytes to scan
 * @param Buffer     Destination buffer for the null-terminated output
 * @param BufferSize Size of Buffer in bytes; must be at least 1
 *
 * @return PE_ASCII_STRING_STATUS Result indicating whether the string was read successfully
 */
static PE_ASCII_STRING_STATUS
PeReadAsciiStringAtRva(PPE_IMAGE_READER Reader, DWORD Rva, DWORD MaxLength, CHAR * Buffer, SIZE_T BufferSize)
{
    if (Buffer == NULL || BufferSize == 0 || MaxLength == 0)
    {
        return PeAsciiStringInvalid;
    }

    Buffer[0] = '\0';

    SIZE_T OutputIndex = 0;
    for (DWORD Index = 0; Index < MaxLength; Index++)
    {
        DWORD        CharacterRva = 0;
        const BYTE * Character    = NULL;

        if (!PeAddDword(Rva, Index, &CharacterRva) || !PeGetPointerAtRva(Reader, CharacterRva, 1, &Character))
        {
            return PeAsciiStringInvalid;
        }

        if (*Character == '\0')
        {
            return PeAsciiStringOk;
        }

        if (OutputIndex + 1 < BufferSize)
        {
            Buffer[OutputIndex++] = (*Character >= 0x20 && *Character <= 0x7e) ? (CHAR)*Character : '.';
            Buffer[OutputIndex]   = '\0';
        }
    }

    return PeAsciiStringTruncated;
}

/**
 * @brief Returns a human-readable description of a PE_ASCII_STRING_STATUS value
 *
 * @param Status The status value returned by PeReadAsciiStringAtRva or related functions
 *
 * @return const CHAR* A static string describing the status
 */
static const CHAR *
PeAsciiStatusName(PE_ASCII_STRING_STATUS Status)
{
    switch (Status)
    {
    case PeAsciiStringInvalid:
        return "invalid mapping";
    case PeAsciiStringTruncated:
        return "truncated or unterminated";
    default:
        return "readable";
    }
}

/**
 * @brief Converts an absolute virtual address to a relative virtual address
 *
 * Subtracts ImageBase from Va and checks that the difference fits in a DWORD.
 * Returns FALSE when Va < ImageBase or when the difference exceeds MAXDWORD.
 *
 * @param Va        Absolute virtual address to convert
 * @param ImageBase Base address of the image
 * @param Rva       Output pointer that receives the RVA on success; must not be NULL
 *
 * @return BOOLEAN TRUE on success, FALSE when the conversion is not representable
 */
static BOOLEAN
PeVaToRva(ULONGLONG Va, ULONGLONG ImageBase, DWORD * Rva)
{
    ULONGLONG Difference = 0;

    if (Rva == NULL || Va < ImageBase)
    {
        return FALSE;
    }

    Difference = Va - ImageBase;
    if (Difference > MAXDWORD)
    {
        return FALSE;
    }

    *Rva = (DWORD)Difference;
    return TRUE;
}

/**
 * @brief Reads a DWORD from a byte buffer at the specified byte offset
 *
 * Uses CopyMemory to avoid strict-aliasing and alignment issues.
 *
 * @param Buffer Pointer to the source byte buffer
 * @param Offset Byte offset within Buffer at which to read
 *
 * @return DWORD The 32-bit value read from Buffer + Offset
 */
static DWORD
PeReadDwordFromBuffer(const BYTE * Buffer, SIZE_T Offset)
{
    DWORD Value = 0;

    CopyMemory(&Value, Buffer + Offset, sizeof(Value));
    return Value;
}

/**
 * @brief Reads a WORD from a byte buffer at the specified byte offset
 *
 * Uses CopyMemory to avoid strict-aliasing and alignment issues.
 *
 * @param Buffer Pointer to the source byte buffer
 * @param Offset Byte offset within Buffer at which to read
 *
 * @return WORD The 16-bit value read from Buffer + Offset
 */
static WORD
PeReadWordFromBuffer(const BYTE * Buffer, SIZE_T Offset)
{
    WORD Value = 0;

    CopyMemory(&Value, Buffer + Offset, sizeof(Value));
    return Value;
}

/**
 * @brief Reads a BYTE from a byte buffer at the specified byte offset
 *
 * Uses CopyMemory to provide a consistent read interface.
 *
 * @param Buffer Pointer to the source byte buffer
 * @param Offset Byte offset within Buffer at which to read
 *
 * @return BYTE The 8-bit value read from Buffer + Offset
 */
static BYTE
PeReadByteFromBuffer(const BYTE * Buffer, SIZE_T Offset)
{
    BYTE Value = 0;

    CopyMemory(&Value, Buffer + Offset, sizeof(Value));
    return Value;
}

/**
 * @brief Reads a ULONGLONG from a byte buffer at the specified byte offset
 *
 * Uses CopyMemory to avoid strict-aliasing and alignment issues.
 *
 * @param Buffer Pointer to the source byte buffer
 * @param Offset Byte offset within Buffer at which to read
 *
 * @return ULONGLONG The 64-bit value read from Buffer + Offset
 */
static ULONGLONG
PeReadQwordFromBuffer(const BYTE * Buffer, SIZE_T Offset)
{
    ULONGLONG Value = 0;

    CopyMemory(&Value, Buffer + Offset, sizeof(Value));
    return Value;
}

/**
 * @brief Reads a pointer-sized value from a load configuration structure
 *
 * Reads a DWORD when Is32Bit is TRUE (PE32 pointer), or a QWORD when FALSE
 * (PE32+ pointer), returning the result as a ULONGLONG in both cases.
 *
 * @param Buffer  Pointer to the load configuration byte buffer
 * @param Offset  Byte offset within Buffer of the field to read
 * @param Is32Bit TRUE to read a 32-bit pointer, FALSE to read a 64-bit pointer
 *
 * @return ULONGLONG The pointer value widened to 64 bits
 */
static ULONGLONG
PeReadLoadConfigPointer(const BYTE * Buffer, SIZE_T Offset, BOOLEAN Is32Bit)
{
    return Is32Bit ? PeReadDwordFromBuffer(Buffer, Offset) : PeReadQwordFromBuffer(Buffer, Offset);
}

/**
 * @brief Reads an ASCII string from a raw byte pointer up to MaxLength bytes
 *
 * Scans StringPointer[0..MaxLength-1] for a null terminator. Non-printable
 * bytes are replaced with '.'. The output is always null-terminated.
 * Returns PeAsciiStringOk when the terminator is found, PeAsciiStringTruncated
 * when MaxLength is exhausted without a terminator, and PeAsciiStringInvalid
 * on NULL arguments.
 *
 * @param StringPointer Pointer to the source ASCII data; must not be NULL
 * @param MaxLength     Maximum number of bytes to scan
 * @param Buffer        Destination buffer for the null-terminated output
 * @param BufferSize    Size of Buffer in bytes; must be at least 1
 *
 * @return PE_ASCII_STRING_STATUS Result indicating whether the string was read successfully
 */
static PE_ASCII_STRING_STATUS
PeReadAsciiStringFromBuffer(const BYTE * StringPointer, DWORD MaxLength, CHAR * Buffer, SIZE_T BufferSize)
{
    if (StringPointer == NULL || Buffer == NULL || BufferSize == 0 || MaxLength == 0)
    {
        return PeAsciiStringInvalid;
    }

    Buffer[0] = '\0';

    SIZE_T OutputIndex = 0;
    for (DWORD Index = 0; Index < MaxLength; Index++)
    {
        if (StringPointer[Index] == '\0')
        {
            return PeAsciiStringOk;
        }

        if (OutputIndex + 1 < BufferSize)
        {
            Buffer[OutputIndex++] = (StringPointer[Index] >= 0x20 && StringPointer[Index] <= 0x7e) ? (CHAR)StringPointer[Index] : '.';
            Buffer[OutputIndex]   = '\0';
        }
    }

    return PeAsciiStringTruncated;
}

/**
 * @brief Reads an ASCII string located at a fixed offset within a PE data directory
 *
 * Computes the target RVA as Directory->VirtualAddress + Offset, clamps the
 * maximum read length to the remaining bytes in the directory, and delegates
 * to PeReadAsciiStringAtRva.
 *
 * @param Reader     Pointer to an initialized PE_IMAGE_READER
 * @param Directory  Pointer to the data directory entry that contains the string
 * @param Offset     Byte offset within the directory at which the string begins
 * @param MaxLength  Maximum number of bytes to scan for the null terminator
 * @param Buffer     Destination buffer for the null-terminated output
 * @param BufferSize Size of Buffer in bytes
 *
 * @return PE_ASCII_STRING_STATUS Result indicating whether the string was read successfully
 */
static PE_ASCII_STRING_STATUS
PeReadAsciiStringInDirectory(PPE_IMAGE_READER             Reader,
                             const IMAGE_DATA_DIRECTORY * Directory,
                             DWORD                        Offset,
                             DWORD                        MaxLength,
                             CHAR *                       Buffer,
                             SIZE_T                       BufferSize)
{
    DWORD StringRva = 0;

    if (Directory == NULL || Offset >= Directory->Size || !PeAddDword(Directory->VirtualAddress, Offset, &StringRva))
    {
        return PeAsciiStringInvalid;
    }

    DWORD Remaining = Directory->Size - Offset;
    return PeReadAsciiStringAtRva(Reader, StringRva, Remaining < MaxLength ? Remaining : MaxLength, Buffer, BufferSize);
}

/**
 * @brief Computes the standard PE file checksum
 *
 * Implements the algorithm used by the Windows PE loader: sums all 16-bit
 * words of the image (treating the checksum field itself as zero), folds the
 * carry, and adds the total image size. The result matches the value stored
 * in the optional header CheckSum field for well-formed images.
 *
 * @param Reader         Pointer to an initialized PE_IMAGE_READER
 * @param ChecksumOffset Raw file offset of the 4-byte checksum field (excluded from summation)
 * @param Checksum       Output pointer that receives the computed checksum on success
 *
 * @return BOOLEAN TRUE on success, FALSE on NULL arguments or invalid checksum offset
 */
static BOOLEAN
PeComputeChecksum(PPE_IMAGE_READER Reader, SIZE_T ChecksumOffset, DWORD * Checksum)
{
    ULONGLONG Sum = 0;

    if (Reader == NULL || Checksum == NULL || ChecksumOffset > Reader->ImageSize || sizeof(DWORD) > Reader->ImageSize - ChecksumOffset)
    {
        return FALSE;
    }

    for (SIZE_T Offset = 0; Offset < Reader->ImageSize; Offset += sizeof(WORD))
    {
        WORD Value = 0;

        if (Offset >= ChecksumOffset && Offset < ChecksumOffset + sizeof(DWORD))
        {
            Value = 0;
        }
        else if (Offset + 1 < Reader->ImageSize)
        {
            CopyMemory(&Value, Reader->ImageBase + Offset, sizeof(Value));
        }
        else
        {
            Value = Reader->ImageBase[Offset];
        }

        Sum += Value;
        Sum = (Sum & 0xffff) + (Sum >> 16);
    }

    Sum       = (Sum & 0xffff) + (Sum >> 16);
    *Checksum = (DWORD)Sum + (DWORD)Reader->ImageSize;
    return TRUE;
}

/**
 * @brief Prints the stored and computed PE optional header checksums
 *
 * Displays HeaderChecksum as read from the optional header and, when the image
 * is small enough, also computes and displays the expected value via
 * PeComputeChecksum. Skips computation for images larger than 64 MiB.
 *
 * @param Reader          Pointer to an initialized PE_IMAGE_READER (may be NULL to skip computation)
 * @param HeaderChecksum  Checksum value as stored in the PE optional header
 * @param ChecksumOffset  Raw file offset of the checksum field within the image
 */
static VOID
PeShowChecksum(PPE_IMAGE_READER Reader, DWORD HeaderChecksum, SIZE_T ChecksumOffset)
{
    const SIZE_T MaxChecksumBytes = 64 * 1024 * 1024;
    DWORD        ComputedChecksum = 0;

    ShowMessages("\n%-36s%#x", "Optional header checksum :", HeaderChecksum);
    if (Reader != NULL && Reader->ImageSize > MaxChecksumBytes)
    {
        ShowMessages("\n%-36s%s", "Computed PE checksum :", "skipped, file is larger than local cap");
        return;
    }

    if (PeComputeChecksum(Reader, ChecksumOffset, &ComputedChecksum))
    {
        ShowMessages("\n%-36s%#x (%s)", "Computed PE checksum :", ComputedChecksum, ComputedChecksum == HeaderChecksum ? "matches" : "differs");
    }
    else
    {
        ShowMessages("\n%-36s%s", "Computed PE checksum :", "not available");
    }
}

/**
 * @brief Prints the contents of the PE certificate (security) directory
 *
 * The certificate directory is stored at a raw file offset rather than an RVA.
 * Iterates WIN_CERTIFICATE entries, printing the file offset, length, revision,
 * and type of each. Stops when the directory is empty, invalid, or a certificate
 * length or alignment error is detected.
 *
 * @param Reader             Pointer to an initialized PE_IMAGE_READER
 * @param SecurityDirectory  Pointer to the IMAGE_DIRECTORY_ENTRY_SECURITY data directory entry
 */
static VOID
PeShowCertificateTable(PPE_IMAGE_READER Reader, const IMAGE_DATA_DIRECTORY * SecurityDirectory)
{
    const DWORD MaxCertificates = 0x1000;

    ShowMessages("\n\nCertificate table\n-----------------");

    if (SecurityDirectory->VirtualAddress == 0 || SecurityDirectory->Size == 0)
    {
        ShowMessages("\n%-36s%s", "Certificate table :", "empty");
        return;
    }

    const BYTE * DirectoryPointer = NULL;
    if (!PeImageReaderGetPointerAtOffset(Reader, SecurityDirectory->VirtualAddress, SecurityDirectory->Size, &DirectoryPointer))
    {
        ShowMessages("\n%-36s%s", "Certificate table :", "invalid bounds");
        return;
    }

    DWORD Offset = 0;
    DWORD Count  = 0;
    while (Offset < SecurityDirectory->Size && Count < MaxCertificates)
    {
        if (SecurityDirectory->Size - Offset < 8)
        {
            ShowMessages("\n%-36s%#llx", "Warning, truncated certificate at :", (UINT64)SecurityDirectory->VirtualAddress + Offset);
            break;
        }

        const BYTE * Entry    = DirectoryPointer + Offset;
        DWORD        Length   = PeReadDwordFromBuffer(Entry, 0);
        WORD         Revision = PeReadWordFromBuffer(Entry, 4);
        WORD         Type     = PeReadWordFromBuffer(Entry, 6);

        ShowMessages("\n[%u] file offset %#llx length %#x revision %#x type %#x", Count, (UINT64)SecurityDirectory->VirtualAddress + Offset, Length, Revision, Type);

        if (Length < 8)
        {
            ShowMessages("\n%-36s%s", "Warning :", "certificate length is smaller than header");
            break;
        }

        if (Length > SecurityDirectory->Size - Offset)
        {
            ShowMessages("\n%-36s%s", "Warning :", "certificate entry extends past directory");
            break;
        }

        DWORD AlignedLength = (Length + 7) & ~7u;
        if (AlignedLength < Length || AlignedLength > SecurityDirectory->Size - Offset)
        {
            ShowMessages("\n%-36s%s", "Warning :", "certificate alignment extends past directory");
            break;
        }

        Offset += AlignedLength;
        Count++;
    }

    ShowMessages("\n%-36s%u", "Certificate entry count :", Count);
    if (Count == MaxCertificates)
    {
        ShowMessages("\n%-36s%#x", "Warning, certificate cap reached :", MaxCertificates);
    }
}

/**
 * @brief Prints base relocation blocks and per-type entry counts
 *
 * Iterates IMAGE_BASE_RELOCATION blocks within the .reloc directory,
 * printing the page RVA, block size, and entry count for each. Accumulates
 * a per-type summary and emits warnings for malformed or oversized data.
 * Processing stops at 0x10000 blocks or 0x100000 entries.
 *
 * @param Reader          Pointer to an initialized PE_IMAGE_READER
 * @param RelocDirectory  Pointer to the IMAGE_DIRECTORY_ENTRY_BASERELOC data directory entry
 */
static VOID
PeShowBaseRelocations(PPE_IMAGE_READER Reader, const IMAGE_DATA_DIRECTORY * RelocDirectory)
{
    const DWORD MaxBlocks        = 0x10000;
    const DWORD MaxEntries       = 0x100000;
    const DWORD MaxPrintedBlocks = 0x20;
    DWORD       TypeCounts[16]   = {0};
    DWORD       BlockCount       = 0;
    ULONGLONG   EntryCount       = 0;
    BOOLEAN     EntryCapped      = FALSE;

    ShowMessages("\n\nBase relocations\n----------------");

    if (RelocDirectory->VirtualAddress == 0 || RelocDirectory->Size == 0)
    {
        ShowMessages("\n%-36s%s", "Base relocation table :", "empty");
        return;
    }

    DWORD Offset = 0;
    while (Offset < RelocDirectory->Size && BlockCount < MaxBlocks && EntryCount < MaxEntries)
    {
        DWORD BlockRva = 0;
        if (!PeAddDword(RelocDirectory->VirtualAddress, Offset, &BlockRva) || RelocDirectory->Size - Offset < sizeof(IMAGE_BASE_RELOCATION))
        {
            ShowMessages("\n%-36s%s", "Warning :", "relocation block header is truncated");
            break;
        }

        const BYTE * BlockPointer = NULL;
        if (!PeGetPointerAtRva(Reader, BlockRva, sizeof(IMAGE_BASE_RELOCATION), &BlockPointer))
        {
            ShowMessages("\n%-36s%#x", "Warning, invalid relocation block RVA :", BlockRva);
            break;
        }

        DWORD PageRva     = PeReadDwordFromBuffer(BlockPointer, 0);
        DWORD SizeOfBlock = PeReadDwordFromBuffer(BlockPointer, 4);

        if (SizeOfBlock < sizeof(IMAGE_BASE_RELOCATION))
        {
            ShowMessages("\n%-36s%s", "Warning :", "relocation block size is smaller than header");
            break;
        }

        if (SizeOfBlock > RelocDirectory->Size - Offset)
        {
            ShowMessages("\n%-36s%s", "Warning :", "relocation block extends past directory");
            break;
        }

        if (((SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) & 1) != 0)
        {
            ShowMessages("\n%-36s%#x", "Warning, odd relocation block size :", SizeOfBlock);
        }

        DWORD EntriesInBlock = (SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
        if (BlockCount < MaxPrintedBlocks)
        {
            ShowMessages("\n[%u] page RVA %#x block size %#x entries %u", BlockCount, PageRva, SizeOfBlock, EntriesInBlock);
        }

        DWORD EntriesRva = 0;
        if (!PeAddDword(BlockRva, sizeof(IMAGE_BASE_RELOCATION), &EntriesRva))
        {
            ShowMessages("\n%-36s%s", "Warning :", "relocation entries RVA overflow");
            break;
        }

        const BYTE * Entries = NULL;
        if (EntriesInBlock != 0 && !PeGetPointerAtRva(Reader, EntriesRva, EntriesInBlock * sizeof(WORD), &Entries))
        {
            ShowMessages("\n%-36s%#x", "Warning, invalid relocation entries RVA :", EntriesRva);
            break;
        }

        for (DWORD EntryIndex = 0; EntryIndex < EntriesInBlock && EntryCount < MaxEntries; EntryIndex++)
        {
            WORD Entry = PeReadWordFromBuffer(Entries, EntryIndex * sizeof(WORD));
            WORD Type  = (Entry >> 12) & 0xf;

            TypeCounts[Type]++;
            EntryCount++;
            if (EntryCount == MaxEntries)
            {
                EntryCapped = TRUE;
                break;
            }

            if (Type == IMAGE_REL_BASED_HIGHADJ)
            {
                if (EntryIndex + 1 >= EntriesInBlock)
                {
                    ShowMessages("\n%-36s%s", "Warning :", "HIGHADJ relocation is missing its pair entry");
                }
                else
                {
                    EntryIndex++;
                    EntryCount++;
                }
            }
        }

        if (EntryCount == MaxEntries)
        {
            EntryCapped = TRUE;
        }

        Offset += SizeOfBlock;
        BlockCount++;
    }

    ShowMessages("\n%-36s%u", "Relocation block count :", BlockCount);
    ShowMessages("\n%-36s%llu", "Relocation entry count :", EntryCount);
    for (DWORD Type = 0; Type < RTL_NUMBER_OF(TypeCounts); Type++)
    {
        if (TypeCounts[Type] != 0)
        {
            ShowMessages("\n%-36s%u = %u", "Relocation type count :", Type, TypeCounts[Type]);
        }
    }

    if (BlockCount == MaxBlocks)
    {
        ShowMessages("\n%-36s%#x", "Warning, relocation block cap reached :", MaxBlocks);
    }
    if (EntryCapped)
    {
        ShowMessages("\n%-36s%#x", "Warning, relocation entry cap reached :", MaxEntries);
    }
    if (BlockCount > MaxPrintedBlocks)
    {
        ShowMessages("\n%-36s%#x", "Info, printed relocation blocks :", MaxPrintedBlocks);
    }
}

/**
 * @brief Prints bound import descriptors and their forwarder references
 *
 * Iterates IMAGE_BOUND_IMPORT_DESCRIPTOR entries, printing the module name,
 * timestamp, and forwader reference count for each. Also expands forwader
 * references inline. Processing stops after 0x1000 descriptors.
 *
 * @param Reader               Pointer to an initialized PE_IMAGE_READER
 * @param BoundImportDirectory Pointer to the IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT data directory entry
 */
static VOID
PeShowBoundImports(PPE_IMAGE_READER Reader, const IMAGE_DATA_DIRECTORY * BoundImportDirectory)
{
    const DWORD MaxDescriptors = 0x1000;
    const DWORD MaxNameLength  = 0x200;

    ShowMessages("\n\nBound imports\n-------------");

    if (BoundImportDirectory->VirtualAddress == 0 || BoundImportDirectory->Size == 0)
    {
        ShowMessages("\n%-36s%s", "Bound import directory :", "empty");
        return;
    }

    DWORD Offset          = 0;
    DWORD DescriptorCount = 0;
    while (Offset < BoundImportDirectory->Size && DescriptorCount < MaxDescriptors)
    {
        DWORD DescriptorRva = 0;
        if (!PeAddDword(BoundImportDirectory->VirtualAddress, Offset, &DescriptorRva) || BoundImportDirectory->Size - Offset < sizeof(IMAGE_BOUND_IMPORT_DESCRIPTOR))
        {
            ShowMessages("\n%-36s%s", "Warning :", "bound import descriptor is truncated");
            break;
        }

        const BYTE * Descriptor = NULL;
        if (!PeGetPointerAtRva(Reader, DescriptorRva, sizeof(IMAGE_BOUND_IMPORT_DESCRIPTOR), &Descriptor))
        {
            ShowMessages("\n%-36s%#x", "Warning, invalid bound descriptor RVA :", DescriptorRva);
            break;
        }

        DWORD TimeDateStamp            = PeReadDwordFromBuffer(Descriptor, 0);
        WORD  OffsetModuleName         = PeReadWordFromBuffer(Descriptor, 4);
        WORD  NumberOfModuleForwarders = PeReadWordFromBuffer(Descriptor, 6);

        if (TimeDateStamp == 0 && OffsetModuleName == 0 && NumberOfModuleForwarders == 0)
        {
            break;
        }

        CHAR                   ModuleName[MaxNameLength + 1] = {0};
        PE_ASCII_STRING_STATUS NameStatus                    = PeReadAsciiStringInDirectory(Reader,
                                                                         BoundImportDirectory,
                                                                         OffsetModuleName,
                                                                         MaxNameLength,
                                                                         ModuleName,
                                                                         sizeof(ModuleName));

        ShowMessages("\n[%u] module %s timestamp %#x forwarder refs %u",
                     DescriptorCount,
                     NameStatus == PeAsciiStringOk ? ModuleName : PeAsciiStatusName(NameStatus),
                     TimeDateStamp,
                     NumberOfModuleForwarders);

        DWORD DescriptorSpan = sizeof(IMAGE_BOUND_IMPORT_DESCRIPTOR) + (NumberOfModuleForwarders * (DWORD)sizeof(IMAGE_BOUND_FORWARDER_REF));
        if (DescriptorSpan > BoundImportDirectory->Size - Offset)
        {
            ShowMessages("\n%-36s%s", "Warning :", "bound import forwarder refs extend past directory");
            break;
        }

        for (WORD ForwarderIndex = 0; ForwarderIndex < NumberOfModuleForwarders; ForwarderIndex++)
        {
            DWORD ForwarderOffset = Offset + sizeof(IMAGE_BOUND_IMPORT_DESCRIPTOR) + (ForwarderIndex * (DWORD)sizeof(IMAGE_BOUND_FORWARDER_REF));
            DWORD ForwarderRva    = 0;

            if (!PeAddDword(BoundImportDirectory->VirtualAddress, ForwarderOffset, &ForwarderRva))
            {
                ShowMessages("\n%-36s%s", "Warning :", "bound forwarder RVA overflow");
                break;
            }

            const BYTE * Forwarder = NULL;
            if (!PeGetPointerAtRva(Reader, ForwarderRva, sizeof(IMAGE_BOUND_FORWARDER_REF), &Forwarder))
            {
                ShowMessages("\n%-36s%#x", "Warning, invalid bound forwarder RVA :", ForwarderRva);
                break;
            }

            DWORD ForwarderTimeDateStamp           = PeReadDwordFromBuffer(Forwarder, 0);
            WORD  ForwarderOffsetModuleName        = PeReadWordFromBuffer(Forwarder, 4);
            CHAR  ForwarderName[MaxNameLength + 1] = {0};

            PE_ASCII_STRING_STATUS ForwarderNameStatus = PeReadAsciiStringInDirectory(Reader,
                                                                                      BoundImportDirectory,
                                                                                      ForwarderOffsetModuleName,
                                                                                      MaxNameLength,
                                                                                      ForwarderName,
                                                                                      sizeof(ForwarderName));
            ShowMessages("\n    [%u] forwarder %s timestamp %#x",
                         ForwarderIndex,
                         ForwarderNameStatus == PeAsciiStringOk ? ForwarderName : PeAsciiStatusName(ForwarderNameStatus),
                         ForwarderTimeDateStamp);
        }

        Offset += DescriptorSpan;
        DescriptorCount++;
    }

    ShowMessages("\n%-36s%u", "Bound import descriptor count :", DescriptorCount);
    if (DescriptorCount == MaxDescriptors)
    {
        ShowMessages("\n%-36s%#x", "Warning, bound import cap reached :", MaxDescriptors);
    }
}

/**
 * @brief Computes the 64-bit FNV-1a hash of a byte buffer
 *
 * Uses the standard FNV-1a parameters: offset basis 14695981039346656037
 * and prime 1099511628211. Suitable for non-cryptographic fingerprinting.
 *
 * @param Data Pointer to the input data
 * @param Size Number of bytes to hash
 *
 * @return ULONGLONG The 64-bit FNV-1a hash value
 */
static ULONGLONG
PeFnv1a64(const BYTE * Data, DWORD Size)
{
    ULONGLONG Hash = 14695981039346656037ull;

    for (DWORD Index = 0; Index < Size; Index++)
    {
        Hash ^= Data[Index];
        Hash *= 1099511628211ull;
    }

    return Hash;
}

/**
 * @brief Computes the Shannon entropy of a byte buffer
 *
 * Calculates the frequency of each byte value (0-255) and applies the
 * standard information-entropy formula. A return value near 8.0 indicates
 * high entropy (e.g. compressed or encrypted data); values near 0.0 indicate
 * low entropy (e.g. sparse, mostly-zero data).
 *
 * @param Data Pointer to the input data
 * @param Size Number of bytes to analyse
 *
 * @return double Shannon entropy in bits per byte (0.0 to 8.0)
 */
static double
PeCalculateEntropy(const BYTE * Data, DWORD Size)
{
    DWORD  Counts[256] = {0};
    double Entropy     = 0.0;

    for (DWORD Index = 0; Index < Size; Index++)
    {
        Counts[Data[Index]]++;
    }

    for (DWORD Index = 0; Index < RTL_NUMBER_OF(Counts); Index++)
    {
        if (Counts[Index] != 0)
        {
            double Probability = (double)Counts[Index] / (double)Size;
            Entropy -= Probability * (log(Probability) / log(2.0));
        }
    }

    return Entropy;
}

/**
 * @brief Reads a 16-bit word from the image at the location mapped by an RVA
 *
 * @param Reader Pointer to an initialized PE_IMAGE_READER
 * @param Rva    Relative virtual address to read from
 * @param Value  Output pointer that receives the WORD value on success; must not be NULL
 *
 * @return BOOLEAN TRUE on success, FALSE if the RVA cannot be resolved or Value is NULL
 */
static BOOLEAN
PeReadWordAtRva(PPE_IMAGE_READER Reader, DWORD Rva, WORD * Value)
{
    const BYTE * Pointer = NULL;

    if (Value == NULL || !PeGetPointerAtRva(Reader, Rva, sizeof(WORD), &Pointer))
    {
        return FALSE;
    }

    CopyMemory(Value, Pointer, sizeof(*Value));
    return TRUE;
}

/**
 * @brief Reads an import/export thunk value (DWORD or QWORD) from an RVA
 *
 * Reads sizeof(DWORD) bytes when Is32Bit is TRUE, or sizeof(ULONGLONG) bytes
 * when FALSE, and widens the result to a ULONGLONG.
 *
 * @param Reader  Pointer to an initialized PE_IMAGE_READER
 * @param Rva     Relative virtual address of the thunk entry
 * @param Is32Bit TRUE to read a 32-bit thunk, FALSE to read a 64-bit thunk
 * @param Value   Output pointer that receives the thunk value on success; must not be NULL
 *
 * @return BOOLEAN TRUE on success, FALSE if the RVA cannot be resolved or Value is NULL
 */
static BOOLEAN
PeReadThunkAtRva(PPE_IMAGE_READER Reader, DWORD Rva, BOOLEAN Is32Bit, ULONGLONG * Value)
{
    const BYTE * Pointer = NULL;
    DWORD        Size    = Is32Bit ? sizeof(DWORD) : sizeof(ULONGLONG);

    if (Value == NULL || !PeGetPointerAtRva(Reader, Rva, Size, &Pointer))
    {
        return FALSE;
    }

    if (Is32Bit)
    {
        DWORD Value32 = 0;
        CopyMemory(&Value32, Pointer, sizeof(Value32));
        *Value = Value32;
    }
    else
    {
        CopyMemory(Value, Pointer, sizeof(*Value));
    }

    return TRUE;
}

/**
 * @brief Prints a named resource type identifier and its occurrence count
 *
 * Maps well-known RT_* integer type identifiers (1-12, 14, 16, 24) to
 * human-readable names. Unknown or unnamed type IDs are silently ignored.
 *
 * @param TypeId Numeric resource type identifier (RT_* value)
 * @param Count  Number of resources of this type found in the resource directory
 */
static VOID
PeShowResourceTypeCount(DWORD TypeId, DWORD Count)
{
    const CHAR * TypeName = NULL;

    switch (TypeId)
    {
    case 1:
        TypeName = "cursor";
        break;
    case 2:
        TypeName = "bitmap";
        break;
    case 3:
        TypeName = "icon";
        break;
    case 4:
        TypeName = "menu";
        break;
    case 5:
        TypeName = "dialog";
        break;
    case 6:
        TypeName = "string";
        break;
    case 7:
        TypeName = "font directory";
        break;
    case 8:
        TypeName = "font";
        break;
    case 9:
        TypeName = "accelerator";
        break;
    case 10:
        TypeName = "rcdata";
        break;
    case 11:
        TypeName = "message table";
        break;
    case 12:
        TypeName = "group cursor";
        break;
    case 14:
        TypeName = "group icon";
        break;
    case 16:
        TypeName = "version";
        break;
    case 24:
        TypeName = "manifest";
        break;
    default:
        TypeName = NULL;
        break;
    }

    if (TypeName != NULL)
    {
        ShowMessages("\n%-36s%s (%u) = %u", "Resource type count :", TypeName, TypeId, Count);
    }
}

/**
 * @brief Traverses and prints the PE resource directory tree
 *
 * Performs an iterative depth-first walk of the three-level resource directory
 * (type / name or id / language), printing directory and data-entry statistics.
 * Accumulates per-type counts for well-known resource types and reports total
 * declared and mapped data bytes. Processing is capped at 0x4000 nodes and
 * 0x10000 entries to prevent runaway on corrupt images.
 *
 * @param Reader             Pointer to an initialized PE_IMAGE_READER
 * @param ResourceDirectory  Pointer to the IMAGE_DIRECTORY_ENTRY_RESOURCE data directory entry
 */
static VOID
PeShowResources(PPE_IMAGE_READER Reader, const IMAGE_DATA_DIRECTORY * ResourceDirectory)
{
    const DWORD MaxDepth          = 8;
    const DWORD MaxNodes          = 0x4000;
    const DWORD MaxEntries        = 0x10000;
    const DWORD MaxVisitedOffsets = 0x4000;

    typedef struct _RESOURCE_NODE
    {
        DWORD Offset;
        DWORD Depth;
        DWORD TypeId;
    } RESOURCE_NODE;

    RESOURCE_NODE * Stack                  = NULL;
    DWORD *         Visited                = NULL;
    DWORD           StackCount             = 0;
    DWORD           VisitedCount           = 0;
    DWORD           TypeCounts[25]         = {0};
    DWORD           DirectoryCount         = 0;
    DWORD           EntryCount             = 0;
    DWORD           DeclaredDataEntryCount = 0;
    DWORD           MappedDataEntryCount   = 0;
    DWORD           InvalidEntryCount      = 0;
    DWORD           NamedRootTypeCount     = 0;
    ULONGLONG       DeclaredDataBytes      = 0;
    ULONGLONG       MappedDataBytes        = 0;
    BOOLEAN         NodeCapReached         = FALSE;
    BOOLEAN         EntryCapReached        = FALSE;

    ShowMessages("\n\nResources\n---------");

    if (ResourceDirectory->VirtualAddress == 0 || ResourceDirectory->Size == 0)
    {
        ShowMessages("\n%-36s%s", "Resource directory :", "empty");
        return;
    }

    if (ResourceDirectory->Size < 16)
    {
        ShowMessages("\n%-36s%s", "Warning :", "resource directory is smaller than one header");
        return;
    }

    Stack   = new (std::nothrow) RESOURCE_NODE[MaxNodes];
    Visited = new (std::nothrow) DWORD[MaxVisitedOffsets];

    if (Stack == NULL || Visited == NULL)
    {
        ShowMessages("\n%-36s%s", "Resource directory :", "memory allocation failed");
        delete[] Stack;
        delete[] Visited;
        return;
    }

    ZeroMemory(Stack, sizeof(RESOURCE_NODE) * MaxNodes);
    ZeroMemory(Visited, sizeof(DWORD) * MaxVisitedOffsets);

    Stack[StackCount].Offset = 0;
    Stack[StackCount].Depth  = 0;
    Stack[StackCount].TypeId = 0;
    StackCount++;

    while (StackCount != 0)
    {
        RESOURCE_NODE Node = Stack[--StackCount];

        if (DirectoryCount >= MaxNodes)
        {
            NodeCapReached = TRUE;
            break;
        }

        if (Node.Depth > MaxDepth)
        {
            InvalidEntryCount++;
            continue;
        }

        BOOLEAN Seen = FALSE;
        for (DWORD Index = 0; Index < VisitedCount; Index++)
        {
            if (Visited[Index] == Node.Offset)
            {
                Seen = TRUE;
                break;
            }
        }

        if (Seen)
        {
            InvalidEntryCount++;
            continue;
        }

        if (VisitedCount < MaxVisitedOffsets)
        {
            Visited[VisitedCount++] = Node.Offset;
        }
        else
        {
            NodeCapReached = TRUE;
            break;
        }

        DWORD DirectoryRva = 0;
        if (!PeAddDword(ResourceDirectory->VirtualAddress, Node.Offset, &DirectoryRva) ||
            Node.Offset > ResourceDirectory->Size || ResourceDirectory->Size - Node.Offset < 16)
        {
            InvalidEntryCount++;
            continue;
        }

        const BYTE * Directory = NULL;
        if (!PeGetPointerAtRva(Reader, DirectoryRva, 16, &Directory))
        {
            InvalidEntryCount++;
            continue;
        }

        WORD  NamedEntries = PeReadWordFromBuffer(Directory, 12);
        WORD  IdEntries    = PeReadWordFromBuffer(Directory, 14);
        DWORD EntryTotal   = (DWORD)NamedEntries + (DWORD)IdEntries;
        DWORD EntryBytes   = 0;

        if (EntryTotal > (MAXDWORD - 16) / 8 || !PeAddDword(16, EntryTotal * 8, &EntryBytes) ||
            EntryBytes > ResourceDirectory->Size - Node.Offset)
        {
            InvalidEntryCount++;
            continue;
        }

        DirectoryCount++;

        const BYTE * Entries = NULL;
        if (EntryTotal != 0 && !PeGetPointerAtRva(Reader, DirectoryRva, EntryBytes, &Entries))
        {
            InvalidEntryCount++;
            continue;
        }

        for (DWORD EntryIndex = 0; EntryIndex < EntryTotal; EntryIndex++)
        {
            if (EntryCount >= MaxEntries)
            {
                EntryCapReached = TRUE;
                break;
            }

            const BYTE * Entry        = Entries + 16 + (EntryIndex * 8);
            DWORD        NameOrId     = PeReadDwordFromBuffer(Entry, 0);
            DWORD        OffsetToData = PeReadDwordFromBuffer(Entry, 4);
            DWORD        ChildOffset  = OffsetToData & 0x7fffffff;
            DWORD        TypeId       = Node.TypeId;

            EntryCount++;

            if ((NameOrId & 0x80000000) != 0)
            {
                DWORD StringOffset = NameOrId & 0x7fffffff;
                DWORD StringRva    = 0;

                if (StringOffset > ResourceDirectory->Size || ResourceDirectory->Size - StringOffset < sizeof(WORD) ||
                    !PeAddDword(ResourceDirectory->VirtualAddress, StringOffset, &StringRva))
                {
                    InvalidEntryCount++;
                }
                else
                {
                    const BYTE * String = NULL;
                    if (!PeGetPointerAtRva(Reader, StringRva, sizeof(WORD), &String))
                    {
                        InvalidEntryCount++;
                    }
                    else
                    {
                        WORD  StringLength = PeReadWordFromBuffer(String, 0);
                        DWORD StringBytes  = (DWORD)sizeof(WORD) + ((DWORD)StringLength * sizeof(WCHAR));

                        if (StringBytes > ResourceDirectory->Size - StringOffset || !PeGetPointerAtRva(Reader, StringRva, StringBytes, &String))
                        {
                            InvalidEntryCount++;
                        }
                    }
                }
                if (Node.Depth == 0)
                {
                    NamedRootTypeCount++;
                }
            }
            else if (Node.Depth == 0)
            {
                TypeId = NameOrId & 0xffff;
                if (TypeId < RTL_NUMBER_OF(TypeCounts))
                {
                    TypeCounts[TypeId]++;
                }
            }

            if ((OffsetToData & 0x80000000) != 0)
            {
                if (ChildOffset > ResourceDirectory->Size || ResourceDirectory->Size - ChildOffset < 16 || Node.Depth + 1 > MaxDepth)
                {
                    InvalidEntryCount++;
                    continue;
                }

                if (StackCount >= MaxNodes)
                {
                    NodeCapReached = TRUE;
                    continue;
                }

                Stack[StackCount].Offset = ChildOffset;
                Stack[StackCount].Depth  = Node.Depth + 1;
                Stack[StackCount].TypeId = TypeId;
                StackCount++;
            }
            else
            {
                DWORD DataEntryRva = 0;
                if (ChildOffset > ResourceDirectory->Size || ResourceDirectory->Size - ChildOffset < 16 ||
                    !PeAddDword(ResourceDirectory->VirtualAddress, ChildOffset, &DataEntryRva))
                {
                    InvalidEntryCount++;
                    continue;
                }

                const BYTE * DataEntry = NULL;
                if (!PeGetPointerAtRva(Reader, DataEntryRva, 16, &DataEntry))
                {
                    InvalidEntryCount++;
                    continue;
                }

                DWORD DataRva  = PeReadDwordFromBuffer(DataEntry, 0);
                DWORD DataSize = PeReadDwordFromBuffer(DataEntry, 4);

                BOOLEAN PayloadMapped = TRUE;
                if (DataSize != 0)
                {
                    const BYTE * Payload = NULL;
                    if (!PeGetPointerAtRva(Reader, DataRva, DataSize, &Payload))
                    {
                        InvalidEntryCount++;
                        PayloadMapped = FALSE;
                    }
                }

                DeclaredDataBytes += DataSize;
                DeclaredDataEntryCount++;
                if (PayloadMapped)
                {
                    MappedDataBytes += DataSize;
                    MappedDataEntryCount++;
                }
            }
        }

        if (EntryCapReached)
        {
            break;
        }
    }

    ShowMessages("\n%-36s%u", "Resource directory count :", DirectoryCount);
    ShowMessages("\n%-36s%u", "Resource entry count :", EntryCount);
    ShowMessages("\n%-36s%u", "Resource declared data entries :", DeclaredDataEntryCount);
    ShowMessages("\n%-36s%u", "Resource mapped data entries :", MappedDataEntryCount);
    ShowMessages("\n%-36s%u", "Resource invalid entry count :", InvalidEntryCount);
    ShowMessages("\n%-36s%u", "Resource named root types :", NamedRootTypeCount);
    ShowMessages("\n%-36s%#llx", "Resource declared data bytes :", DeclaredDataBytes);
    ShowMessages("\n%-36s%#llx", "Resource mapped data bytes :", MappedDataBytes);

    for (DWORD TypeId = 0; TypeId < RTL_NUMBER_OF(TypeCounts); TypeId++)
    {
        if (TypeCounts[TypeId] != 0)
        {
            PeShowResourceTypeCount(TypeId, TypeCounts[TypeId]);
        }
    }

    if (NodeCapReached)
    {
        ShowMessages("\n%-36s%#x", "Warning, resource node cap reached :", MaxNodes);
    }
    if (EntryCapReached)
    {
        ShowMessages("\n%-36s%#x", "Warning, resource entry cap reached :", MaxEntries);
    }

    delete[] Stack;
    delete[] Visited;
}

/**
 * @brief Prints RUNTIME_FUNCTION entries from the x64 exception directory
 *
 * Decodes IMAGE_RUNTIME_FUNCTION_ENTRY records for AMD64 images, printing
 * the begin address, end address, unwind RVA, and key unwind info fields
 * for the first 0x20 entries. For non-AMD64 machines the directory size
 * is reported without per-entry decoding. Processing is capped at 0x100000
 * entries.
 *
 * @param Reader             Pointer to an initialized PE_IMAGE_READER
 * @param ExceptionDirectory Pointer to the IMAGE_DIRECTORY_ENTRY_EXCEPTION data directory entry
 * @param Machine            Machine type from the PE file header (IMAGE_FILE_MACHINE_*)
 * @param Is32Bit            TRUE if the PE image is 32-bit
 */
static VOID
PeShowExceptions(PPE_IMAGE_READER Reader, const IMAGE_DATA_DIRECTORY * ExceptionDirectory, WORD Machine, BOOLEAN Is32Bit)
{
    const DWORD RuntimeFunctionSize = 12;
    const DWORD MaxEntries          = 0x100000;
    const DWORD MaxPrintedEntries   = 0x20;
    const DWORD MaxWarnings         = 8;

    ShowMessages("\n\nExceptions\n----------");

    if (ExceptionDirectory->VirtualAddress == 0 || ExceptionDirectory->Size == 0)
    {
        ShowMessages("\n%-36s%s", "Exception directory :", "empty");
        return;
    }

    if (Machine != IMAGE_FILE_MACHINE_AMD64 || Is32Bit)
    {
        ShowMessages("\n%-36s%s", "Exception directory :", "present, x64 runtime-function decoding skipped for this machine");
        ShowMessages("\n%-36s%#x", "Raw exception directory size :", ExceptionDirectory->Size);
        return;
    }

    if (ExceptionDirectory->Size < RuntimeFunctionSize)
    {
        ShowMessages("\n%-36s%s", "Warning :", "exception directory is smaller than one runtime function entry");
        return;
    }

    if ((ExceptionDirectory->Size % RuntimeFunctionSize) != 0)
    {
        ShowMessages("\n%-36s%#x", "Warning, malformed exception size :", ExceptionDirectory->Size);
    }

    DWORD   EntryCount         = ExceptionDirectory->Size / RuntimeFunctionSize;
    BOOLEAN EntryCapped        = FALSE;
    DWORD   RangeWarnings      = 0;
    DWORD   UnwindWarnings     = 0;
    DWORD   SkippedNullEntries = 0;
    DWORD   PrintedEntries     = 0;
    if (EntryCount > MaxEntries)
    {
        EntryCount  = MaxEntries;
        EntryCapped = TRUE;
    }

    ShowMessages("\n%-36s%u", "Runtime function count :", EntryCount);

    for (DWORD EntryIndex = 0; EntryIndex < EntryCount; EntryIndex++)
    {
        DWORD EntryRva = 0;
        if (!PeAddDword(ExceptionDirectory->VirtualAddress, EntryIndex * RuntimeFunctionSize, &EntryRva))
        {
            ShowMessages("\n%-36s%s", "Warning :", "runtime function entry RVA overflow");
            break;
        }

        const BYTE * Entry = NULL;
        if (!PeGetPointerAtRva(Reader, EntryRva, RuntimeFunctionSize, &Entry))
        {
            ShowMessages("\n%-36s%#x", "Warning, invalid runtime entry RVA :", EntryRva);
            break;
        }

        DWORD BeginAddress = PeReadDwordFromBuffer(Entry, 0);
        DWORD EndAddress   = PeReadDwordFromBuffer(Entry, 4);
        DWORD UnwindRva    = PeReadDwordFromBuffer(Entry, 8);

        if (BeginAddress == 0 && EndAddress == 0 && UnwindRva == 0)
        {
            SkippedNullEntries++;
            continue;
        }

        if (BeginAddress >= EndAddress)
        {
            if (RangeWarnings < MaxWarnings && PrintedEntries < MaxPrintedEntries)
            {
                ShowMessages("\n%-36s%u", "Warning, invalid runtime range index :", EntryIndex);
            }
            RangeWarnings++;
        }

        const BYTE * Unwind       = NULL;
        BOOLEAN      UnwindMapped = UnwindRva != 0 && PeGetPointerAtRva(Reader, UnwindRva, 4, &Unwind);
        if (!UnwindMapped)
        {
            if (UnwindWarnings < MaxWarnings && PrintedEntries < MaxPrintedEntries)
            {
                ShowMessages("\n%-36s%#x", "Warning, invalid unwind RVA :", UnwindRva);
            }
            UnwindWarnings++;
        }

        if (PrintedEntries < MaxPrintedEntries)
        {
            if (UnwindMapped)
            {
                BYTE VersionAndFlags = PeReadByteFromBuffer(Unwind, 0);
                BYTE Frame           = PeReadByteFromBuffer(Unwind, 3);

                ShowMessages("\n[%u] begin %#x end %#x unwind %#x version %u flags %#x prolog %#x codes %u frame reg %u frame off %u",
                             EntryIndex,
                             BeginAddress,
                             EndAddress,
                             UnwindRva,
                             VersionAndFlags & 0x7,
                             VersionAndFlags >> 3,
                             PeReadByteFromBuffer(Unwind, 1),
                             PeReadByteFromBuffer(Unwind, 2),
                             Frame & 0xf,
                             Frame >> 4);
            }
            else
            {
                ShowMessages("\n[%u] begin %#x end %#x unwind %#x", EntryIndex, BeginAddress, EndAddress, UnwindRva);
            }
            PrintedEntries++;
        }
    }

    if (EntryCapped)
    {
        ShowMessages("\n%-36s%#x", "Warning, exception entry cap reached :", MaxEntries);
    }
    if (EntryCount > MaxPrintedEntries)
    {
        ShowMessages("\n%-36s%u", "Info, printed exception entries :", PrintedEntries);
    }
    if (SkippedNullEntries != 0)
    {
        ShowMessages("\n%-36s%u", "Info, skipped null runtime entries :", SkippedNullEntries);
    }
    if (RangeWarnings > MaxWarnings)
    {
        ShowMessages("\n%-36s%#x", "Warning, invalid range warnings shown :", MaxWarnings);
    }
    if (UnwindWarnings > MaxWarnings)
    {
        ShowMessages("\n%-36s%#x", "Warning, invalid unwind warnings shown :", MaxWarnings);
    }
}

/**
 * @brief Resolves a delay-import address field to a relative virtual address
 *
 * Handles both the legacy VA-based format (UsesRva == FALSE) and the newer
 * RVA-based format (UsesRva == TRUE). For VA-based descriptors the address is
 * converted via PeVaToRva using the supplied image base.
 *
 * @param Address    Raw address value read from the delay-import descriptor
 * @param UsesRva    TRUE if the descriptor uses RVAs, FALSE if it uses VAs
 * @param ImageBase  Image base used for VA-to-RVA conversion when UsesRva is FALSE
 * @param Rva        Output pointer that receives the resolved RVA on success; must not be NULL
 *
 * @return BOOLEAN TRUE on success, FALSE when the address cannot be converted to an RVA
 */
static BOOLEAN
PeResolveDelayImportAddress(ULONGLONG Address, BOOLEAN UsesRva, ULONGLONG ImageBase, DWORD * Rva)
{
    if (Rva == NULL)
    {
        return FALSE;
    }

    if (UsesRva)
    {
        if (Address > MAXDWORD)
        {
            return FALSE;
        }

        *Rva = (DWORD)Address;
        return TRUE;
    }

    return PeVaToRva(Address, ImageBase, Rva);
}

/**
 * @brief Prints delay-import descriptors and their imported function names or ordinals
 *
 * Iterates ImgDelayDescr records, resolving DLL names and import thunk arrays.
 * Supports both legacy VA-based and RVA-based descriptor formats. Processing
 * is capped at 0x1000 descriptors and 0x2000 total imported symbols.
 *
 * @param Reader              Pointer to an initialized PE_IMAGE_READER
 * @param DelayImportDirectory Pointer to the IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT data directory entry
 * @param Is32Bit             TRUE if the PE image is 32-bit
 * @param ImageBase           Preferred image base used for VA-to-RVA conversion
 */
static VOID
PeShowDelayImports(PPE_IMAGE_READER Reader, const IMAGE_DATA_DIRECTORY * DelayImportDirectory, BOOLEAN Is32Bit, ULONGLONG ImageBase)
{
    const DWORD MaxDescriptors      = 0x1000;
    const DWORD MaxTotalImports     = 0x2000;
    const DWORD MaxDllNameLength    = 0x200;
    const DWORD MaxImportNameLength = 0x200;
    const DWORD DescriptorSize      = 32;

    ShowMessages("\n\nDelay imports\n-------------");

    if (DelayImportDirectory->VirtualAddress == 0 || DelayImportDirectory->Size == 0)
    {
        ShowMessages("\n%-36s%s", "Delay import directory :", "empty");
        return;
    }

    if (DelayImportDirectory->Size < DescriptorSize)
    {
        ShowMessages("\n%-36s%s", "Warning :", "delay import directory is smaller than one descriptor");
        return;
    }

    DWORD DescriptorCount = DelayImportDirectory->Size / DescriptorSize;
    if ((DelayImportDirectory->Size % DescriptorSize) != 0)
    {
        ShowMessages("\n%-36s%#x", "Warning, malformed delay import size :", DelayImportDirectory->Size);
    }

    BOOLEAN DescriptorCapped = FALSE;
    if (DescriptorCount > MaxDescriptors)
    {
        DescriptorCount  = MaxDescriptors;
        DescriptorCapped = TRUE;
    }

    DWORD   TotalImports       = 0;
    BOOLEAN FoundTerminator    = FALSE;
    BOOLEAN TotalImportsCapped = FALSE;

    for (DWORD DescriptorIndex = 0; DescriptorIndex < DescriptorCount; DescriptorIndex++)
    {
        DWORD DescriptorRva = 0;
        if (!PeAddDword(DelayImportDirectory->VirtualAddress, DescriptorIndex * DescriptorSize, &DescriptorRva))
        {
            ShowMessages("\n%-36s%s", "Warning :", "delay import descriptor RVA overflow");
            break;
        }

        const BYTE * Descriptor = NULL;
        if (!PeGetPointerAtRva(Reader, DescriptorRva, DescriptorSize, &Descriptor))
        {
            ShowMessages("\n%-36s%#x", "Warning, invalid delay descriptor RVA :", DescriptorRva);
            break;
        }

        DWORD Attributes     = PeReadDwordFromBuffer(Descriptor, 0);
        DWORD NameValue      = PeReadDwordFromBuffer(Descriptor, 4);
        DWORD IatValue       = PeReadDwordFromBuffer(Descriptor, 12);
        DWORD IntValue       = PeReadDwordFromBuffer(Descriptor, 16);
        DWORD BoundIatValue  = PeReadDwordFromBuffer(Descriptor, 20);
        DWORD UnloadIatValue = PeReadDwordFromBuffer(Descriptor, 24);
        DWORD TimeDateStamp  = PeReadDwordFromBuffer(Descriptor, 28);

        if (Attributes == 0 && NameValue == 0 && PeReadDwordFromBuffer(Descriptor, 8) == 0 && IatValue == 0 && IntValue == 0 &&
            BoundIatValue == 0 && UnloadIatValue == 0 && TimeDateStamp == 0)
        {
            FoundTerminator = TRUE;
            break;
        }

        BOOLEAN UsesRva        = (Attributes & 1) != 0;
        DWORD   NameRva        = 0;
        DWORD   LookupThunkRva = 0;
        DWORD   IatThunkRva    = 0;

        if (!UsesRva)
        {
            ShowMessages("\n%-36s%s", "Info :", "delay import descriptor uses VA fields");
        }

        if (!PeResolveDelayImportAddress(NameValue, UsesRva, ImageBase, &NameRva))
        {
            ShowMessages("\n[%u] DLL name %s", DescriptorIndex, "invalid VA/RVA");
            ShowMessages("\n%-36s%s", "Warning :", "skipping delay imports for descriptor with unreadable DLL name");
            continue;
        }

        CHAR                   DllName[MaxDllNameLength + 1] = {0};
        PE_ASCII_STRING_STATUS DllNameStatus                 = PeReadAsciiStringAtRva(Reader,
                                                                      NameRva,
                                                                      MaxDllNameLength,
                                                                      DllName,
                                                                      sizeof(DllName));
        if (DllNameStatus != PeAsciiStringOk)
        {
            ShowMessages("\n[%u] DLL name %s", DescriptorIndex, PeAsciiStatusName(DllNameStatus));
            ShowMessages("\n%-36s%s", "Warning :", "skipping delay imports for descriptor with unreadable DLL name");
            continue;
        }

        ShowMessages("\n[%u] DLL name %s attributes %#x timestamp %#x", DescriptorIndex, DllName, Attributes, TimeDateStamp);

        if (!PeResolveDelayImportAddress(IntValue != 0 ? IntValue : IatValue, UsesRva, ImageBase, &LookupThunkRva) ||
            !PeResolveDelayImportAddress(IatValue, UsesRva, ImageBase, &IatThunkRva))
        {
            ShowMessages("\n%-36s%s", "Warning :", "delay import thunk VA/RVA is invalid");
            continue;
        }

        DWORD ThunkSize = Is32Bit ? sizeof(DWORD) : sizeof(ULONGLONG);
        for (DWORD ThunkIndex = 0; TotalImports < MaxTotalImports; ThunkIndex++)
        {
            DWORD LookupEntryRva = 0;
            DWORD IatEntryRva    = 0;
            if (!PeAddDword(LookupThunkRva, ThunkIndex * ThunkSize, &LookupEntryRva) ||
                !PeAddDword(IatThunkRva, ThunkIndex * ThunkSize, &IatEntryRva))
            {
                ShowMessages("\n%-36s%s", "Warning :", "delay import thunk RVA overflow");
                break;
            }

            ULONGLONG ThunkValue = 0;
            if (!PeReadThunkAtRva(Reader, LookupEntryRva, Is32Bit, &ThunkValue))
            {
                ShowMessages("\n%-36s%#x", "Warning, invalid delay thunk RVA :", LookupEntryRva);
                break;
            }

            if (ThunkValue == 0)
            {
                break;
            }

            ULONGLONG OrdinalFlag = Is32Bit ? IMAGE_ORDINAL_FLAG32 : IMAGE_ORDINAL_FLAG64;
            if ((ThunkValue & OrdinalFlag) != 0)
            {
                ShowMessages("\n    [%u] ordinal %u thunk RVA %#x", ThunkIndex, (UINT32)(ThunkValue & 0xffff), IatEntryRva);
                TotalImports++;
                continue;
            }

            DWORD NameRvaFromThunk = 0;
            if (!PeResolveDelayImportAddress(ThunkValue, UsesRva, ImageBase, &NameRvaFromThunk))
            {
                ShowMessages("\n%-36s%#llx", "Warning, invalid delay import name VA/RVA :", (UINT64)ThunkValue);
                TotalImports++;
                continue;
            }

            WORD Hint = 0;
            if (!PeReadWordAtRva(Reader, NameRvaFromThunk, &Hint))
            {
                ShowMessages("\n%-36s%#x", "Warning, invalid delay import hint RVA :", NameRvaFromThunk);
                TotalImports++;
                continue;
            }

            DWORD ImportNameRva = 0;
            if (!PeAddDword(NameRvaFromThunk, sizeof(WORD), &ImportNameRva))
            {
                ShowMessages("\n%-36s%#x", "Warning, invalid delay import name RVA :", NameRvaFromThunk);
                TotalImports++;
                continue;
            }

            CHAR                   ImportName[MaxImportNameLength + 1] = {0};
            PE_ASCII_STRING_STATUS ImportNameStatus                    = PeReadAsciiStringAtRva(Reader,
                                                                             ImportNameRva,
                                                                             MaxImportNameLength,
                                                                             ImportName,
                                                                             sizeof(ImportName));
            if (ImportNameStatus != PeAsciiStringOk)
            {
                ShowMessages("\n%-36s%#x, %s", "Warning, invalid delay import name RVA :", ImportNameRva, PeAsciiStatusName(ImportNameStatus));
                TotalImports++;
                continue;
            }

            ShowMessages("\n    [%u] hint %#x name %s thunk RVA %#x", ThunkIndex, Hint, ImportName, IatEntryRva);
            TotalImports++;
        }

        if (TotalImports >= MaxTotalImports)
        {
            TotalImportsCapped = TRUE;
            break;
        }
    }

    if (!FoundTerminator)
    {
        ShowMessages("\n%-36s%s", "Warning :", "delay import descriptor terminator not found before bounds or cap");
    }
    if (DescriptorCapped)
    {
        ShowMessages("\n%-36s%#x", "Warning, delay descriptor cap reached :", MaxDescriptors);
    }
    if (TotalImportsCapped)
    {
        ShowMessages("\n%-36s%#x", "Warning, delay import cap reached :", MaxTotalImports);
    }
}

/**
 * @brief Prints the RVA and size of an IMAGE_DATA_DIRECTORY embedded within the CLR runtime header
 *
 * Reads two consecutive DWORDs from the CLR header buffer at Offset to obtain
 * a sub-directory RVA and size. Also reports whether the range is mapped
 * within the image.
 *
 * @param Reader        Pointer to an initialized PE_IMAGE_READER
 * @param Header        Pointer to the CLR runtime header byte buffer
 * @param AvailableSize Number of validated bytes available in Header
 * @param Offset        Byte offset within Header of the IMAGE_DATA_DIRECTORY to read
 * @param Label         Column label to print before the values
 */
static VOID
PeShowClrDirectoryBounds(PPE_IMAGE_READER Reader, const BYTE * Header, DWORD AvailableSize, SIZE_T Offset, const CHAR * Label)
{
    if (Offset > AvailableSize || sizeof(IMAGE_DATA_DIRECTORY) > AvailableSize - Offset)
    {
        return;
    }

    DWORD Rva  = PeReadDwordFromBuffer(Header, Offset);
    DWORD Size = PeReadDwordFromBuffer(Header, Offset + sizeof(DWORD));

    ShowMessages("\n%-36sRVA %#x size %#x", Label, Rva, Size);
    if (Rva != 0 && Size != 0)
    {
        const BYTE * Pointer = NULL;
        ShowMessages(", %s", PeGetPointerAtRva(Reader, Rva, Size, &Pointer) ? "mapped" : "not mapped");
    }
}

/**
 * @brief Prints CLR (.NET) runtime header information
 *
 * Reads the IMAGE_COR20_HEADER from the CLR data directory, printing the
 * runtime version, flags, entry point token or RVA, and the bounds of each
 * embedded sub-directory (metadata, resources, strong name, etc.).
 *
 * @param Reader       Pointer to an initialized PE_IMAGE_READER
 * @param ClrDirectory Pointer to the IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR data directory entry
 */
static VOID
PeShowClrRuntime(PPE_IMAGE_READER Reader, const IMAGE_DATA_DIRECTORY * ClrDirectory)
{
    ShowMessages("\n\nCLR runtime\n-----------");

    if (ClrDirectory->VirtualAddress == 0 || ClrDirectory->Size == 0)
    {
        ShowMessages("\n%-36s%s", "CLR runtime header :", "empty");
        return;
    }

    if (ClrDirectory->Size < 0x48)
    {
        ShowMessages("\n%-36s%s", "Warning :", "CLR runtime header is smaller than required fields");
        return;
    }

    const BYTE * Header = NULL;
    if (!PeGetPointerAtRva(Reader, ClrDirectory->VirtualAddress, 0x48, &Header))
    {
        ShowMessages("\n%-36s%s", "CLR runtime header :", "not mapped");
        return;
    }

    DWORD HeaderSize    = PeReadDwordFromBuffer(Header, 0);
    DWORD AvailableSize = ClrDirectory->Size < HeaderSize ? ClrDirectory->Size : HeaderSize;

    ShowMessages("\n%-36s%#x", "Header size :", HeaderSize);
    if (HeaderSize < 0x48 || AvailableSize < 0x48)
    {
        ShowMessages("\n%-36s%s", "Warning :", "CLR runtime header is truncated");
        return;
    }

    if (!PeGetPointerAtRva(Reader, ClrDirectory->VirtualAddress, AvailableSize, &Header))
    {
        ShowMessages("\n%-36s%s", "CLR runtime header :", "invalid bounds");
        return;
    }

    WORD  MajorRuntimeVersion = PeReadWordFromBuffer(Header, 4);
    WORD  MinorRuntimeVersion = PeReadWordFromBuffer(Header, 6);
    DWORD Flags               = PeReadDwordFromBuffer(Header, 16);
    DWORD EntryPoint          = PeReadDwordFromBuffer(Header, 20);

    ShowMessages("\n%-36s%u.%u", "Runtime version :", MajorRuntimeVersion, MinorRuntimeVersion);
    PeShowClrDirectoryBounds(Reader, Header, AvailableSize, 8, "Metadata directory :");
    ShowMessages("\n%-36s%#x", "Flags :", Flags);
    ShowMessages("\n%-36s%#x (%s)", "Entry point token/RVA :", EntryPoint, (Flags & 0x10) != 0 ? "native RVA" : "managed token");
    PeShowClrDirectoryBounds(Reader, Header, AvailableSize, 24, "Resources directory :");
    PeShowClrDirectoryBounds(Reader, Header, AvailableSize, 32, "Strong name directory :");
    PeShowClrDirectoryBounds(Reader, Header, AvailableSize, 40, "Code manager table :");
    PeShowClrDirectoryBounds(Reader, Header, AvailableSize, 48, "VTable fixups :");
    PeShowClrDirectoryBounds(Reader, Header, AvailableSize, 56, "Export address table jumps :");
    PeShowClrDirectoryBounds(Reader, Header, AvailableSize, 64, "Managed native header :");
}

/**
 * @brief Prints Thread Local Storage (TLS) directory fields and TLS callback addresses
 *
 * Reads IMAGE_TLS_DIRECTORY32 or IMAGE_TLS_DIRECTORY64 depending on Is32Bit and
 * prints all fields. Then walks the null-terminated callback VA array, converting
 * each entry to an RVA and file offset. Processing stops at 0x200 callbacks.
 *
 * @param Reader       Pointer to an initialized PE_IMAGE_READER
 * @param TlsDirectory Pointer to the IMAGE_DIRECTORY_ENTRY_TLS data directory entry
 * @param Is32Bit      TRUE if the PE image is 32-bit
 * @param ImageBase    Preferred image base used for VA-to-RVA conversion
 */
static VOID
PeShowTls(PPE_IMAGE_READER Reader, const IMAGE_DATA_DIRECTORY * TlsDirectory, BOOLEAN Is32Bit, ULONGLONG ImageBase)
{
    const DWORD MaxCallbacks = 0x200;
    DWORD       TlsSize      = Is32Bit ? sizeof(IMAGE_TLS_DIRECTORY32) : sizeof(IMAGE_TLS_DIRECTORY64);

    ShowMessages("\n\nTLS\n---");

    if (TlsDirectory->VirtualAddress == 0 || TlsDirectory->Size == 0)
    {
        ShowMessages("\n%-36s%s", "TLS directory :", "empty");
        return;
    }

    if (!PeRvaContainsRange(TlsDirectory->VirtualAddress, TlsDirectory->Size, TlsDirectory->VirtualAddress, TlsSize))
    {
        ShowMessages("\n%-36s%s", "TLS directory :", "invalid bounds");
        return;
    }

    const BYTE * TlsPointer = NULL;
    if (!PeGetPointerAtRva(Reader, TlsDirectory->VirtualAddress, TlsSize, &TlsPointer))
    {
        ShowMessages("\n%-36s%s", "TLS directory :", "not mapped");
        return;
    }

    ULONGLONG StartRawDataVa     = Is32Bit ? PeReadDwordFromBuffer(TlsPointer, 0) : PeReadQwordFromBuffer(TlsPointer, 0);
    ULONGLONG EndRawDataVa       = Is32Bit ? PeReadDwordFromBuffer(TlsPointer, 4) : PeReadQwordFromBuffer(TlsPointer, 8);
    ULONGLONG AddressOfIndexVa   = Is32Bit ? PeReadDwordFromBuffer(TlsPointer, 8) : PeReadQwordFromBuffer(TlsPointer, 16);
    ULONGLONG AddressCallbacksVa = Is32Bit ? PeReadDwordFromBuffer(TlsPointer, 12) : PeReadQwordFromBuffer(TlsPointer, 24);
    DWORD     SizeOfZeroFill     = Is32Bit ? PeReadDwordFromBuffer(TlsPointer, 16) : PeReadDwordFromBuffer(TlsPointer, 32);
    DWORD     Characteristics    = Is32Bit ? PeReadDwordFromBuffer(TlsPointer, 20) : PeReadDwordFromBuffer(TlsPointer, 36);

    ShowMessages("\n%-36s%#llx", "Start address of raw data VA :", StartRawDataVa);
    ShowMessages("\n%-36s%#llx", "End address of raw data VA :", EndRawDataVa);
    ShowMessages("\n%-36s%#llx", "Address of index VA :", AddressOfIndexVa);
    ShowMessages("\n%-36s%#llx", "Address of callbacks VA :", AddressCallbacksVa);
    ShowMessages("\n%-36s%#x", "Size of zero fill :", SizeOfZeroFill);
    ShowMessages("\n%-36s%#x", "Characteristics :", Characteristics);

    DWORD CallbacksRva = 0;
    if (AddressCallbacksVa == 0)
    {
        ShowMessages("\n%-36s%s", "TLS callbacks :", "empty");
        return;
    }

    if (!PeVaToRva(AddressCallbacksVa, ImageBase, &CallbacksRva))
    {
        ShowMessages("\n%-36s%#llx", "Warning, invalid callbacks VA :", AddressCallbacksVa);
        return;
    }

    DWORD EntrySize = Is32Bit ? sizeof(DWORD) : sizeof(ULONGLONG);
    for (DWORD CallbackIndex = 0; CallbackIndex < MaxCallbacks; CallbackIndex++)
    {
        DWORD EntryRva = 0;
        if (!PeAddDword(CallbacksRva, CallbackIndex * EntrySize, &EntryRva))
        {
            ShowMessages("\n%-36s%s", "Warning :", "TLS callback RVA overflow");
            return;
        }

        ULONGLONG CallbackVa = 0;
        if (!PeReadThunkAtRva(Reader, EntryRva, Is32Bit, &CallbackVa))
        {
            ShowMessages("\n%-36s%#x", "Warning, invalid callback entry RVA :", EntryRva);
            return;
        }

        if (CallbackVa == 0)
        {
            return;
        }

        DWORD  CallbackRva = 0;
        SIZE_T FileOffset  = 0;
        if (PeVaToRva(CallbackVa, ImageBase, &CallbackRva))
        {
            if (PeImageReaderRvaToFileOffset(Reader, CallbackRva, 1, &FileOffset))
            {
                ShowMessages("\n    [%u] VA %#llx RVA %#x file offset %#llx", CallbackIndex, CallbackVa, CallbackRva, (UINT64)FileOffset);
            }
            else
            {
                ShowMessages("\n    [%u] VA %#llx RVA %#x file offset %s", CallbackIndex, CallbackVa, CallbackRva, "not mapped");
            }
        }
        else
        {
            ShowMessages("\n    [%u] VA %#llx RVA %s", CallbackIndex, CallbackVa, "invalid");
        }
    }

    ShowMessages("\n%-36s%#x", "Warning, TLS callback cap reached :", MaxCallbacks);
}

/**
 * @brief Prints CodeView debug information embedded in a debug directory entry
 *
 * Handles both the RSDS (PDB 7.0) and NB10 (PDB 2.0) CodeView formats.
 * For RSDS entries, prints the GUID, age, and PDB path. For NB10 entries,
 * prints the offset, signature, age, and PDB path. Silently returns for
 * non-CodeView entries or entries with insufficient data.
 *
 * @param Reader Pointer to an initialized PE_IMAGE_READER
 * @param Entry  Pointer to the IMAGE_DEBUG_DIRECTORY entry to decode
 */
static VOID
PeShowDebugCodeView(PPE_IMAGE_READER Reader, const IMAGE_DEBUG_DIRECTORY * Entry)
{
    const DWORD MaxPdbPathLength = 0x400;

    if (Entry->Type != IMAGE_DEBUG_TYPE_CODEVIEW || Entry->SizeOfData < sizeof(DWORD))
    {
        return;
    }

    const BYTE * Payload = NULL;
    if ((Entry->PointerToRawData == 0 || !PeImageReaderGetPointerAtOffset(Reader, Entry->PointerToRawData, Entry->SizeOfData, &Payload)) &&
        (Entry->AddressOfRawData == 0 || !PeGetPointerAtRva(Reader, Entry->AddressOfRawData, Entry->SizeOfData, &Payload)))
    {
        ShowMessages("\n%-36s%s", "    CodeView payload :", "invalid bounds");
        return;
    }

    if (memcmp(Payload, "RSDS", sizeof(DWORD)) == 0)
    {
        if (Entry->SizeOfData < 24)
        {
            ShowMessages("\n%-36s%s", "    CodeView RSDS :", "truncated");
            return;
        }

        DWORD        Age                           = PeReadDwordFromBuffer(Payload, 20);
        DWORD        PathOffset                    = 24;
        DWORD        PathSize                      = Entry->SizeOfData - 24;
        DWORD        PathLimit                     = PathSize < MaxPdbPathLength ? PathSize : MaxPdbPathLength;
        CHAR         PdbPath[MaxPdbPathLength + 1] = {0};
        const BYTE * Guid                          = Payload + 4;

        PE_ASCII_STRING_STATUS PathStatus = PeReadAsciiStringFromBuffer(Payload + PathOffset, PathLimit, PdbPath, sizeof(PdbPath));
        ShowMessages("\n%-36s%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                     "    CodeView RSDS GUID :",
                     Guid[3],
                     Guid[2],
                     Guid[1],
                     Guid[0],
                     Guid[5],
                     Guid[4],
                     Guid[7],
                     Guid[6],
                     Guid[8],
                     Guid[9],
                     Guid[10],
                     Guid[11],
                     Guid[12],
                     Guid[13],
                     Guid[14],
                     Guid[15]);
        ShowMessages("\n%-36s%u", "    CodeView age :", Age);
        ShowMessages("\n%-36s%s", "    CodeView PDB path :", PathStatus == PeAsciiStringOk ? PdbPath : PeAsciiStatusName(PathStatus));
    }
    else if (memcmp(Payload, "NB10", sizeof(DWORD)) == 0)
    {
        if (Entry->SizeOfData < 16)
        {
            ShowMessages("\n%-36s%s", "    CodeView NB10 :", "truncated");
            return;
        }

        DWORD Offset                        = PeReadDwordFromBuffer(Payload, 4);
        DWORD Timestamp                     = PeReadDwordFromBuffer(Payload, 8);
        DWORD Age                           = PeReadDwordFromBuffer(Payload, 12);
        DWORD PathOffset                    = 16;
        DWORD PathSize                      = Entry->SizeOfData - 16;
        DWORD PathLimit                     = PathSize < MaxPdbPathLength ? PathSize : MaxPdbPathLength;
        CHAR  PdbPath[MaxPdbPathLength + 1] = {0};

        PE_ASCII_STRING_STATUS PathStatus = PeReadAsciiStringFromBuffer(Payload + PathOffset, PathLimit, PdbPath, sizeof(PdbPath));
        ShowMessages("\n%-36s%#x", "    CodeView NB10 offset :", Offset);
        ShowMessages("\n%-36s%#x", "    CodeView NB10 signature :", Timestamp);
        ShowMessages("\n%-36s%u", "    CodeView age :", Age);
        ShowMessages("\n%-36s%s", "    CodeView PDB path :", PathStatus == PeAsciiStringOk ? PdbPath : PeAsciiStatusName(PathStatus));
    }
}

/**
 * @brief Prints all IMAGE_DEBUG_DIRECTORY entries in the debug data directory
 *
 * Iterates the debug directory, printing characteristics, timestamp, version,
 * type, size, and address fields for each entry. Also validates the raw data
 * bounds and delegates to PeShowDebugCodeView for CodeView entries.
 * Processing is capped at 0x1000 entries.
 *
 * @param Reader          Pointer to an initialized PE_IMAGE_READER
 * @param DebugDirectory  Pointer to the IMAGE_DIRECTORY_ENTRY_DEBUG data directory entry
 */
static VOID
PeShowDebug(PPE_IMAGE_READER Reader, const IMAGE_DATA_DIRECTORY * DebugDirectory)
{
    const DWORD MaxDebugEntries = 0x1000;

    ShowMessages("\n\nDebug\n-----");

    if (DebugDirectory->VirtualAddress == 0 || DebugDirectory->Size == 0)
    {
        ShowMessages("\n%-36s%s", "Debug directory :", "empty");
        return;
    }

    DWORD EntryCount = DebugDirectory->Size / sizeof(IMAGE_DEBUG_DIRECTORY);
    if (EntryCount == 0)
    {
        ShowMessages("\n%-36s%s", "Warning :", "debug directory is smaller than one descriptor");
        return;
    }

    if ((DebugDirectory->Size % sizeof(IMAGE_DEBUG_DIRECTORY)) != 0)
    {
        ShowMessages("\n%-36s%#x", "Warning, malformed debug size :", DebugDirectory->Size);
    }

    BOOLEAN EntryCapped = FALSE;
    if (EntryCount > MaxDebugEntries)
    {
        EntryCount  = MaxDebugEntries;
        EntryCapped = TRUE;
    }

    for (DWORD EntryIndex = 0; EntryIndex < EntryCount; EntryIndex++)
    {
        DWORD EntryRva = 0;
        if (!PeAddDword(DebugDirectory->VirtualAddress, EntryIndex * (DWORD)sizeof(IMAGE_DEBUG_DIRECTORY), &EntryRva))
        {
            ShowMessages("\n%-36s%s", "Warning :", "debug directory RVA overflow");
            break;
        }

        const IMAGE_DEBUG_DIRECTORY * Entry = NULL;
        if (!PeGetPointerAtRva(Reader, EntryRva, sizeof(IMAGE_DEBUG_DIRECTORY), (const BYTE **)&Entry))
        {
            ShowMessages("\n%-36s%s", "Warning :", "debug directory entry is not mapped");
            break;
        }

        ShowMessages("\n[%u] characteristics %#x time date stamp %#x major %u minor %u type %u size %#x address %#x raw %#x",
                     EntryIndex,
                     Entry->Characteristics,
                     Entry->TimeDateStamp,
                     Entry->MajorVersion,
                     Entry->MinorVersion,
                     Entry->Type,
                     Entry->SizeOfData,
                     Entry->AddressOfRawData,
                     Entry->PointerToRawData);

        if (Entry->SizeOfData != 0 && Entry->PointerToRawData != 0)
        {
            const BYTE * Payload = NULL;
            ShowMessages("\n%-36s%s", "    Payload bounds :", PeImageReaderGetPointerAtOffset(Reader, Entry->PointerToRawData, Entry->SizeOfData, &Payload) ? "valid" : "invalid");
        }

        PeShowDebugCodeView(Reader, Entry);
    }

    if (EntryCapped)
    {
        ShowMessages("\n%-36s%#x", "Warning, debug entry cap reached :", MaxDebugEntries);
    }
}

/**
 * @brief Checks whether a load configuration structure contains a specific field
 *
 * Returns TRUE when the range [Offset, Offset + FieldSize) lies entirely within
 * the first AvailableSize bytes of the load config buffer.
 *
 * @param AvailableSize Number of bytes available in the load config buffer
 * @param Offset        Byte offset of the field within the load config structure
 * @param FieldSize     Size of the field in bytes
 *
 * @return BOOLEAN TRUE if the field is present, FALSE if it is beyond AvailableSize
 */
static BOOLEAN
PeLoadConfigHasField(DWORD AvailableSize, SIZE_T Offset, SIZE_T FieldSize)
{
    return Offset <= AvailableSize && FieldSize <= AvailableSize - Offset;
}

/**
 * @brief Prints a DWORD field from the load config structure if it is present
 *
 * Uses PeLoadConfigHasField to verify the field is within AvailableSize before
 * reading and printing the value with the supplied label.
 *
 * @param Config        Pointer to the load configuration byte buffer
 * @param AvailableSize Number of validated bytes in Config
 * @param Offset        Byte offset of the DWORD field within Config
 * @param Label         Column label to print before the value
 */
static VOID
PeShowLoadConfigDword(const BYTE * Config, DWORD AvailableSize, SIZE_T Offset, const CHAR * Label)
{
    if (PeLoadConfigHasField(AvailableSize, Offset, sizeof(DWORD)))
    {
        ShowMessages("\n%-36s%#x", Label, PeReadDwordFromBuffer(Config, Offset));
    }
}

/**
 * @brief Prints a pointer-sized field from the load config structure if it is present
 *
 * Reads sizeof(DWORD) or sizeof(ULONGLONG) depending on Is32Bit, verifies
 * presence with PeLoadConfigHasField, and prints the value as a hex address.
 *
 * @param Config        Pointer to the load configuration byte buffer
 * @param AvailableSize Number of validated bytes in Config
 * @param Offset        Byte offset of the pointer field within Config
 * @param Label         Column label to print before the value
 * @param Is32Bit       TRUE to read a 32-bit pointer, FALSE for a 64-bit pointer
 */
static VOID
PeShowLoadConfigPointer(const BYTE * Config, DWORD AvailableSize, SIZE_T Offset, const CHAR * Label, BOOLEAN Is32Bit)
{
    SIZE_T FieldSize = Is32Bit ? sizeof(DWORD) : sizeof(ULONGLONG);

    if (PeLoadConfigHasField(AvailableSize, Offset, FieldSize))
    {
        ShowMessages("\n%-36s%#llx", Label, PeReadLoadConfigPointer(Config, Offset, Is32Bit));
    }
}

/**
 * @brief Prints a count field (pointer-sized) from the load config structure if present
 *
 * Behaves identically to PeShowLoadConfigPointer but formats the value as a
 * decimal count rather than a hex address.
 *
 * @param Config        Pointer to the load configuration byte buffer
 * @param AvailableSize Number of validated bytes in Config
 * @param Offset        Byte offset of the count field within Config
 * @param Label         Column label to print before the value
 * @param Is32Bit       TRUE to read a 32-bit count, FALSE for a 64-bit count
 */
static VOID
PeShowLoadConfigCount(const BYTE * Config, DWORD AvailableSize, SIZE_T Offset, const CHAR * Label, BOOLEAN Is32Bit)
{
    SIZE_T FieldSize = Is32Bit ? sizeof(DWORD) : sizeof(ULONGLONG);

    if (PeLoadConfigHasField(AvailableSize, Offset, FieldSize))
    {
        ShowMessages("\n%-36s%llu", Label, PeReadLoadConfigPointer(Config, Offset, Is32Bit));
    }
}

/**
 * @brief Prints the names of set guard control-flow flags from a load config guard flags field
 *
 * Iterates a table of known guard flag bitmasks and prints the name of each
 * set bit. Reports "None" when no known flags are set, and appends the
 * unknown bitmask when unrecognised bits are present.
 *
 * @param GuardFlags The GuardFlags field value from the load configuration structure
 */
static VOID
PeShowLoadConfigGuardFlags(DWORD GuardFlags)
{
    BOOLEAN AnyFlag   = FALSE;
    DWORD   KnownMask = 0;

    struct GUARD_FLAG_NAME
    {
        DWORD        Flag;
        const CHAR * Name;
    };

    static const GUARD_FLAG_NAME GuardFlagNames[] = {
        {0x00000100, "CF instrumented"},
        {0x00000200, "CFW instrumented"},
        {0x00000400, "CF function table present"},
        {0x00000800, "Security cookie unused"},
        {0x00001000, "Protect delayed load IAT"},
        {0x00002000, "Delayed load IAT in its own section"},
        {0x00004000, "Export suppression info present"},
        {0x00008000, "Enable export suppression"},
        {0x00010000, "Long jump table present"},
        {0x00020000, "RF instrumented"},
        {0x00040000, "RF enable"},
        {0x00080000, "RF strict"},
        {0x00100000, "Retpoline present"},
        {0x00200000, "EH continuation table present"},
        {0x00400000, "XFG enabled"},
        {0x00800000, "CastGuard present"},
        {0x01000000, "Memcpy function present"},
    };

    for (UINT32 Index = 0; Index < RTL_NUMBER_OF(GuardFlagNames); Index++)
    {
        KnownMask |= GuardFlagNames[Index].Flag;
        if ((GuardFlags & GuardFlagNames[Index].Flag) == GuardFlagNames[Index].Flag)
        {
            ShowMessages("%s%s", AnyFlag ? ", " : "", GuardFlagNames[Index].Name);
            AnyFlag = TRUE;
        }
    }

    if (!AnyFlag)
    {
        ShowMessages(GuardFlags == 0 ? "None" : "No known guard flags");
    }

    DWORD UnknownMask = GuardFlags & ~KnownMask;
    if (UnknownMask != 0)
    {
        ShowMessages(", unknown mask %#x", UnknownMask);
    }
}

/**
 * @brief Prints extended load config fields introduced in Windows 8.1 and later
 *
 * Handles guard address-taken IAT, guard long-jump tables, dynamic value
 * relocation, CHPE metadata, RF (return-flow) guards, EH continuation tables,
 * XFG, CastGuard, and memcpy pointer fields. Each field is printed only when
 * AvailableSize covers its offset.
 *
 * @param Config        Pointer to the load configuration byte buffer
 * @param AvailableSize Number of validated bytes in Config
 * @param Is32Bit       TRUE for PE32 field offsets, FALSE for PE32+ field offsets
 */
static VOID
PeShowLoadConfigModernFields(const BYTE * Config, DWORD AvailableSize, BOOLEAN Is32Bit)
{
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 104 : 160, "Guard address-taken IAT table :", Is32Bit);
    PeShowLoadConfigCount(Config, AvailableSize, Is32Bit ? 108 : 168, "Guard address-taken IAT count :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 112 : 176, "Guard long jump target table :", Is32Bit);
    PeShowLoadConfigCount(Config, AvailableSize, Is32Bit ? 116 : 184, "Guard long jump target count :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 120 : 192, "Dynamic value reloc table :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 124 : 200, "CHPE metadata pointer :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 128 : 208, "Guard RF failure routine :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 132 : 216, "Guard RF failure routine ptr :", Is32Bit);
    PeShowLoadConfigDword(Config, AvailableSize, Is32Bit ? 136 : 224, "Dynamic value reloc offset :");
    if (PeLoadConfigHasField(AvailableSize, Is32Bit ? 140 : 228, sizeof(WORD)))
    {
        ShowMessages("\n%-36s%#x", "Dynamic value reloc section :", PeReadWordFromBuffer(Config, Is32Bit ? 140 : 228));
    }
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 144 : 232, "Guard RF verify stack ptr :", Is32Bit);
    PeShowLoadConfigDword(Config, AvailableSize, Is32Bit ? 148 : 240, "Hot patch table offset :");
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 156 : 248, "Enclave config pointer :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 160 : 256, "Volatile metadata pointer :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 164 : 264, "Guard EH continuation table :", Is32Bit);
    PeShowLoadConfigCount(Config, AvailableSize, Is32Bit ? 168 : 272, "Guard EH continuation count :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 172 : 280, "Guard XFG check pointer :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 176 : 288, "Guard XFG dispatch pointer :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 180 : 296, "Guard XFG table dispatch ptr :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 184 : 304, "CastGuard failure mode :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 188 : 312, "Guard memcpy pointer :", Is32Bit);
}

/**
 * @brief Prints the load configuration directory
 *
 * Reads the self-describing size field at the start of the load config
 * structure, clamps to the directory bounds, and prints security cookie,
 * SEH handler table, guard CF fields, guard flags, and extended modern
 * fields via helper functions.
 *
 * @param Reader               Pointer to an initialized PE_IMAGE_READER
 * @param LoadConfigDirectory  Pointer to the IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG data directory entry
 * @param Is32Bit              TRUE if the PE image is 32-bit
 */
static VOID
PeShowLoadConfig(PPE_IMAGE_READER Reader, const IMAGE_DATA_DIRECTORY * LoadConfigDirectory, BOOLEAN Is32Bit)
{
    ShowMessages("\n\nLoad config\n-----------");

    if (LoadConfigDirectory->VirtualAddress == 0 || LoadConfigDirectory->Size == 0)
    {
        ShowMessages("\n%-36s%s", "Load config directory :", "empty");
        return;
    }

    if (LoadConfigDirectory->Size < sizeof(DWORD))
    {
        ShowMessages("\n%-36s%s", "Warning :", "load config size is too small");
        return;
    }

    const BYTE * Config = NULL;
    if (!PeGetPointerAtRva(Reader, LoadConfigDirectory->VirtualAddress, sizeof(DWORD), &Config))
    {
        ShowMessages("\n%-36s%s", "Load config directory :", "not mapped");
        return;
    }

    DWORD ConfigSize    = PeReadDwordFromBuffer(Config, 0);
    DWORD AvailableSize = LoadConfigDirectory->Size < ConfigSize ? LoadConfigDirectory->Size : ConfigSize;

    ShowMessages("\n%-36s%#x", "Size :", ConfigSize);
    if (AvailableSize < sizeof(DWORD))
    {
        ShowMessages("\n%-36s%s", "Warning :", "load config size is too small");
        return;
    }

    if (!PeGetPointerAtRva(Reader, LoadConfigDirectory->VirtualAddress, AvailableSize, &Config))
    {
        ShowMessages("\n%-36s%s", "Load config directory :", "invalid bounds");
        return;
    }

    PeShowLoadConfigDword(Config, AvailableSize, 4, "Time date stamp :");
    if (PeLoadConfigHasField(AvailableSize, 8, sizeof(WORD)))
    {
        WORD MajorVersion = 0;
        CopyMemory(&MajorVersion, Config + 8, sizeof(MajorVersion));
        ShowMessages("\n%-36s%u", "Major version :", MajorVersion);
    }
    if (PeLoadConfigHasField(AvailableSize, 10, sizeof(WORD)))
    {
        WORD MinorVersion = 0;
        CopyMemory(&MinorVersion, Config + 10, sizeof(MinorVersion));
        ShowMessages("\n%-36s%u", "Minor version :", MinorVersion);
    }
    PeShowLoadConfigDword(Config, AvailableSize, 12, "Global flags clear :");
    PeShowLoadConfigDword(Config, AvailableSize, 16, "Global flags set :");
    PeShowLoadConfigDword(Config, AvailableSize, 20, "Critical section timeout :");
    PeShowLoadConfigDword(Config, AvailableSize, Is32Bit ? 44 : 72, "Process heap flags :");
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 60 : 88, "Security cookie :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 64 : 96, "SE handler table :", Is32Bit);
    PeShowLoadConfigCount(Config, AvailableSize, Is32Bit ? 68 : 104, "SE handler count :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 72 : 112, "Guard CF check pointer :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 76 : 120, "Guard CF dispatch pointer :", Is32Bit);
    PeShowLoadConfigPointer(Config, AvailableSize, Is32Bit ? 80 : 128, "Guard CF function table :", Is32Bit);
    PeShowLoadConfigCount(Config, AvailableSize, Is32Bit ? 84 : 136, "Guard CF function count :", Is32Bit);
    PeShowLoadConfigDword(Config, AvailableSize, Is32Bit ? 88 : 144, "Guard flags :");
    if (PeLoadConfigHasField(AvailableSize, Is32Bit ? 88 : 144, sizeof(DWORD)))
    {
        DWORD GuardFlags = PeReadDwordFromBuffer(Config, Is32Bit ? 88 : 144);
        ShowMessages("\n%-36s", "Guard flag names :");
        PeShowLoadConfigGuardFlags(GuardFlags);
    }
    PeShowLoadConfigModernFields(Config, AvailableSize, Is32Bit);
}

/**
 * @brief Prints a summary of all PE data directory entries
 *
 * Iterates IMAGE_NUMBEROF_DIRECTORY_ENTRIES slots, printing the name, RVA
 * (or file offset for the certificate entry), size, and mapping status of
 * each. Entries beyond NumberOfRvaAndSizes are marked as "not declared".
 *
 * @param Reader               Pointer to an initialized PE_IMAGE_READER
 * @param Directories          Pointer to the data directory array from the optional header
 * @param NumberOfRvaAndSizes  Value of NumberOfRvaAndSizes from the optional header
 */
static VOID
PeShowDataDirectories(PPE_IMAGE_READER             Reader,
                      const IMAGE_DATA_DIRECTORY * Directories,
                      DWORD                        NumberOfRvaAndSizes)
{
    ShowMessages("\n\nData directories\n----------------");
    ShowMessages("\n%-36s%u", "Number of RVA and sizes :", NumberOfRvaAndSizes);
    if (NumberOfRvaAndSizes > IMAGE_NUMBEROF_DIRECTORY_ENTRIES)
    {
        ShowMessages("\n%-36s%s", "Warning :", "number of RVA and sizes exceeds PE directory table");
    }

    for (UINT32 Index = 0; Index < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; Index++)
    {
        const CHAR * Name         = PeGetDataDirectoryName(Index);
        const CHAR * AddressLabel = Index == IMAGE_DIRECTORY_ENTRY_SECURITY ? "file offset" : "RVA";

        if (Index >= NumberOfRvaAndSizes)
        {
            ShowMessages("\n[%2u] %-27s %s", Index, Name, "not declared");
            continue;
        }

        const IMAGE_DATA_DIRECTORY * Directory = &Directories[Index];
        if (Directory->VirtualAddress == 0 || Directory->Size == 0)
        {
            ShowMessages("\n[%2u] %-27s %s %#x, size %#x, %s",
                         Index,
                         Name,
                         AddressLabel,
                         Directory->VirtualAddress,
                         Directory->Size,
                         "empty");
            continue;
        }

        const BYTE * Pointer    = NULL;
        SIZE_T       FileOffset = 0;

        if (Index == IMAGE_DIRECTORY_ENTRY_SECURITY)
        {
            BOOLEAN HasBounds = PeImageReaderGetPointerAtOffset(Reader,
                                                                Directory->VirtualAddress,
                                                                Directory->Size,
                                                                &Pointer);

            ShowMessages("\n[%2u] %-27s file offset %#x, size %#x, %s",
                         Index,
                         Name,
                         Directory->VirtualAddress,
                         Directory->Size,
                         HasBounds ? "valid bounds" : "invalid bounds");
            if (HasBounds)
            {
                ShowMessages(", mapped file offset %#llx", (UINT64)Directory->VirtualAddress);
            }

            continue;
        }

        if (PeImageReaderRvaToFileOffset(Reader, Directory->VirtualAddress, Directory->Size, &FileOffset))
        {
            ShowMessages("\n[%2u] %-27s RVA %#x, size %#x, mapped file offset %#llx",
                         Index,
                         Name,
                         Directory->VirtualAddress,
                         Directory->Size,
                         (UINT64)FileOffset);
        }
        else
        {
            ShowMessages("\n[%2u] %-27s RVA %#x, size %#x, %s",
                         Index,
                         Name,
                         Directory->VirtualAddress,
                         Directory->Size,
                         "not mapped");
        }
    }
}

/**
 * @brief Prints overlay data information and PE layout warnings
 *
 * Identifies data appended after all mapped sections (overlay), checks for
 * section overlaps and gaps, validates SizeOfImage, and emits warnings for
 * mismatched optional header magic/machine combinations, sections that extend
 * beyond SizeOfImage, and inconsistent section raw data boundaries.
 *
 * @param Reader               Pointer to an initialized PE_IMAGE_READER
 * @param Machine              Machine type from the PE file header
 * @param OptionalHeaderMagic  Magic value from the PE optional header
 * @param AddressOfEntryPoint  AddressOfEntryPoint from the PE optional header
 * @param SizeOfImage          SizeOfImage from the PE optional header
 * @param SizeOfHeaders        SizeOfHeaders from the PE optional header
 * @param Directories          Pointer to the data directory array
 * @param NumberOfRvaAndSizes  Value of NumberOfRvaAndSizes from the optional header
 */
static VOID
PeShowOverlayAndWarnings(PPE_IMAGE_READER             Reader,
                         WORD                         Machine,
                         WORD                         OptionalHeaderMagic,
                         DWORD                        AddressOfEntryPoint,
                         DWORD                        SizeOfImage,
                         DWORD                        SizeOfHeaders,
                         const IMAGE_DATA_DIRECTORY * Directories,
                         DWORD                        NumberOfRvaAndSizes)
{
    ULONGLONG OverlayOffset     = SizeOfHeaders < Reader->ImageSize ? SizeOfHeaders : Reader->ImageSize;
    BOOLEAN   OverlayUnreliable = FALSE;

    ShowMessages("\n\nOverlay and warnings\n--------------------");

    if ((ULONGLONG)SizeOfHeaders > (ULONGLONG)Reader->ImageSize)
    {
        ShowMessages("\n%-36s%s", "Warning :", "size of headers is larger than file size");
    }

    if (OptionalHeaderMagic == IMAGE_NT_OPTIONAL_HDR64_MAGIC && Machine == IMAGE_FILE_MACHINE_I386)
    {
        ShowMessages("\n%-36s%s", "Warning :", "PE32+ optional header with i386 machine type");
    }
    else if (OptionalHeaderMagic == IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
             (Machine == IMAGE_FILE_MACHINE_AMD64 || Machine == IMAGE_FILE_MACHINE_IA64))
    {
        ShowMessages("\n%-36s%s", "Warning :", "PE32 optional header with 64-bit machine type");
    }

    for (UINT32 SectionIndex = 0; SectionIndex < Reader->FileHeader->NumberOfSections; SectionIndex++)
    {
        CHAR                         SectionName[IMAGE_SIZEOF_SHORT_NAME + 1];
        const IMAGE_SECTION_HEADER * Section     = &Reader->SectionHeaders[SectionIndex];
        DWORD                        VirtualSpan = Section->Misc.VirtualSize > Section->SizeOfRawData ? Section->Misc.VirtualSize : Section->SizeOfRawData;
        DWORD                        VirtualEnd  = 0;

        PeImageReaderGetSectionName(Section, SectionName, sizeof(SectionName));

        if (VirtualSpan != 0)
        {
            if (!PeAddDword(Section->VirtualAddress, VirtualSpan, &VirtualEnd))
            {
                ShowMessages("\n%-36ssection '%s' RVA range overflows", "Warning :", SectionName);
            }
            else if (SizeOfImage != 0 && VirtualEnd > SizeOfImage)
            {
                ShowMessages("\n%-36ssection '%s' RVA range exceeds SizeOfImage", "Warning :", SectionName);
            }
        }

        if (Section->SizeOfRawData != 0)
        {
            ULONGLONG RawStart = Section->PointerToRawData;
            ULONGLONG RawEnd   = 0;

            if (!PeAddUlonglong(RawStart, Section->SizeOfRawData, &RawEnd))
            {
                ShowMessages("\n%-36ssection '%s' raw data range overflows", "Warning :", SectionName);
                OverlayUnreliable = TRUE;
            }
            else if (RawStart > (ULONGLONG)Reader->ImageSize || RawEnd > (ULONGLONG)Reader->ImageSize)
            {
                ShowMessages("\n%-36ssection '%s' raw data is outside file", "Warning :", SectionName);
                OverlayUnreliable = TRUE;
            }
            else if (RawEnd > OverlayOffset)
            {
                OverlayOffset = RawEnd;
            }
        }
    }

    if (Reader->FileHeader->NumberOfSections > 1)
    {
        PE_RAW_SECTION_RANGE * Ranges = (PE_RAW_SECTION_RANGE *)malloc(sizeof(PE_RAW_SECTION_RANGE) * Reader->FileHeader->NumberOfSections);
        UINT32                 Count  = 0;

        if (Ranges == NULL)
        {
            ShowMessages("\n%-36s%s", "Warning :", "raw section overlap check skipped, allocation failed");
        }
        else
        {
            for (UINT32 SectionIndex = 0; SectionIndex < Reader->FileHeader->NumberOfSections; SectionIndex++)
            {
                const IMAGE_SECTION_HEADER * Section = &Reader->SectionHeaders[SectionIndex];
                ULONGLONG                    End;

                if (Section->SizeOfRawData == 0 ||
                    !PeAddUlonglong(Section->PointerToRawData, Section->SizeOfRawData, &End) ||
                    Section->PointerToRawData > Reader->ImageSize || End > Reader->ImageSize)
                {
                    continue;
                }

                Ranges[Count].Start   = Section->PointerToRawData;
                Ranges[Count].End     = End;
                Ranges[Count].Section = Section;
                Count++;
            }

            if (Count > 1)
            {
                UINT32 MaxEndIndex = 0;

                qsort(Ranges, Count, sizeof(Ranges[0]), PeCompareRawSectionRange);

                for (UINT32 RangeIndex = 1; RangeIndex < Count; RangeIndex++)
                {
                    if (Ranges[RangeIndex].Start < Ranges[MaxEndIndex].End)
                    {
                        CHAR LeftName[IMAGE_SIZEOF_SHORT_NAME + 1];
                        CHAR RightName[IMAGE_SIZEOF_SHORT_NAME + 1];

                        PeImageReaderGetSectionName(Ranges[MaxEndIndex].Section, LeftName, sizeof(LeftName));
                        PeImageReaderGetSectionName(Ranges[RangeIndex].Section, RightName, sizeof(RightName));
                        ShowMessages("\n%-36sraw data overlap detected between '%s' and '%s'; additional overlaps may be omitted",
                                     "Warning :",
                                     LeftName,
                                     RightName);
                    }

                    if (Ranges[RangeIndex].End > Ranges[MaxEndIndex].End)
                    {
                        MaxEndIndex = RangeIndex;
                    }
                }
            }

            free(Ranges);
        }
    }

    if (AddressOfEntryPoint != 0)
    {
        BOOLEAN                      EntrypointFound   = FALSE;
        const IMAGE_SECTION_HEADER * EntrypointSection = NULL;

        for (UINT32 SectionIndex = 0; SectionIndex < Reader->FileHeader->NumberOfSections; SectionIndex++)
        {
            const IMAGE_SECTION_HEADER * Section     = &Reader->SectionHeaders[SectionIndex];
            DWORD                        VirtualSpan = Section->Misc.VirtualSize > Section->SizeOfRawData ? Section->Misc.VirtualSize : Section->SizeOfRawData;
            DWORD                        VirtualEnd  = 0;

            if (VirtualSpan == 0 || !PeAddDword(Section->VirtualAddress, VirtualSpan, &VirtualEnd))
            {
                continue;
            }

            if (AddressOfEntryPoint >= Section->VirtualAddress && AddressOfEntryPoint < VirtualEnd)
            {
                EntrypointFound   = TRUE;
                EntrypointSection = Section;
                break;
            }
        }

        if (!EntrypointFound)
        {
            ShowMessages("\n%-36s%s", "Warning :", "entrypoint is outside all sections");
        }
        else if ((EntrypointSection->Characteristics & IMAGE_SCN_MEM_EXECUTE) == 0)
        {
            CHAR SectionName[IMAGE_SIZEOF_SHORT_NAME + 1];

            PeImageReaderGetSectionName(EntrypointSection, SectionName, sizeof(SectionName));
            ShowMessages("\n%-36sentrypoint section '%s' is not executable", "Warning :", SectionName);
        }
    }

    if (OverlayUnreliable)
    {
        ShowMessages("\n%-36s%s", "Overlay :", "not computed because section raw data is invalid");
    }
    else if ((ULONGLONG)Reader->ImageSize > OverlayOffset)
    {
        ULONGLONG OverlaySize = (ULONGLONG)Reader->ImageSize - OverlayOffset;

        ShowMessages("\n%-36s%#llx", "Overlay offset :", OverlayOffset);
        ShowMessages("\n%-36s%#llx", "Overlay size :", OverlaySize);

        if (Directories != NULL && NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_SECURITY)
        {
            const IMAGE_DATA_DIRECTORY * SecurityDirectory = &Directories[IMAGE_DIRECTORY_ENTRY_SECURITY];

            if (SecurityDirectory->VirtualAddress != 0 && SecurityDirectory->Size != 0 &&
                (ULONGLONG)SecurityDirectory->VirtualAddress >= OverlayOffset)
            {
                ShowMessages("\n%-36s%s", "Info :", "overlay includes certificate data; not necessarily suspicious");
            }
        }
    }
    else
    {
        ShowMessages("\n%-36s%s", "Overlay :", "none");
    }
}

/**
 * @brief Prints all import descriptors and their imported function names or ordinals
 *
 * Iterates IMAGE_IMPORT_DESCRIPTOR records, resolving DLL names and walking
 * the original-first-thunk (or first-thunk) arrays to print each imported
 * symbol with its hint and IAT RVA. Warns when the null terminator descriptor
 * is not found within the declared directory size. Processing is capped at
 * 0x1000 descriptors and 0x2000 total imports.
 *
 * @param Reader          Pointer to an initialized PE_IMAGE_READER
 * @param ImportDirectory Pointer to the IMAGE_DIRECTORY_ENTRY_IMPORT data directory entry
 * @param Is32Bit         TRUE if the PE image is 32-bit
 */
static VOID
PeShowImports(PPE_IMAGE_READER Reader, const IMAGE_DATA_DIRECTORY * ImportDirectory, BOOLEAN Is32Bit)
{
    const DWORD MaxImportDescriptors = 0x1000;
    const DWORD MaxTotalImports      = 0x2000;
    const DWORD MaxDllNameLength     = 0x200;
    const DWORD MaxImportNameLength  = 0x200;

    ShowMessages("\n\nImports\n-------");

    if (ImportDirectory->VirtualAddress == 0 || ImportDirectory->Size == 0)
    {
        ShowMessages("\n%-36s%s", "Import directory :", "empty");
        return;
    }

    const BYTE * DirectoryPointer = NULL;
    if (!PeGetPointerAtRva(Reader, ImportDirectory->VirtualAddress, sizeof(IMAGE_IMPORT_DESCRIPTOR), &DirectoryPointer))
    {
        ShowMessages("\n%-36s%s", "Import directory :", "not mapped");
        return;
    }

    DWORD DescriptorCount = ImportDirectory->Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);
    if (DescriptorCount == 0)
    {
        ShowMessages("\n%-36s%s", "Warning :", "import directory is smaller than one descriptor");
        return;
    }

    BOOLEAN DescriptorCapped = FALSE;
    if (DescriptorCount > MaxImportDescriptors)
    {
        DescriptorCount  = MaxImportDescriptors;
        DescriptorCapped = TRUE;
    }

    DWORD   TotalImports       = 0;
    BOOLEAN FoundTerminator    = FALSE;
    BOOLEAN TotalImportsCapped = FALSE;

    for (DWORD DescriptorIndex = 0; DescriptorIndex < DescriptorCount; DescriptorIndex++)
    {
        DWORD DescriptorRva = 0;

        if (!PeAddDword(ImportDirectory->VirtualAddress,
                        DescriptorIndex * (DWORD)sizeof(IMAGE_IMPORT_DESCRIPTOR),
                        &DescriptorRva))
        {
            break;
        }

        const IMAGE_IMPORT_DESCRIPTOR * Descriptor = NULL;
        if (!PeGetPointerAtRva(Reader,
                               DescriptorRva,
                               sizeof(IMAGE_IMPORT_DESCRIPTOR),
                               (const BYTE **)&Descriptor))
        {
            ShowMessages("\n%-36s%s", "Warning :", "import descriptor is not mapped");
            break;
        }

        if (Descriptor->OriginalFirstThunk == 0 && Descriptor->TimeDateStamp == 0 && Descriptor->ForwarderChain == 0 &&
            Descriptor->Name == 0 && Descriptor->FirstThunk == 0)
        {
            FoundTerminator = TRUE;
            break;
        }

        CHAR                   DllName[MaxDllNameLength + 1] = {0};
        PE_ASCII_STRING_STATUS DllNameStatus                 = PeReadAsciiStringAtRva(Reader,
                                                                      Descriptor->Name,
                                                                      MaxDllNameLength,
                                                                      DllName,
                                                                      sizeof(DllName));
        if (DllNameStatus != PeAsciiStringOk)
        {
            ShowMessages("\n[%u] DLL name %s", DescriptorIndex, PeAsciiStatusName(DllNameStatus));
            ShowMessages("\n%-36s%s", "Warning :", "skipping imports for descriptor with unreadable DLL name");
            continue;
        }

        ShowMessages("\n[%u] DLL name %s", DescriptorIndex, DllName);

        DWORD LookupThunkRva = Descriptor->OriginalFirstThunk != 0 ? Descriptor->OriginalFirstThunk : Descriptor->FirstThunk;
        if (LookupThunkRva == 0 || Descriptor->FirstThunk == 0)
        {
            ShowMessages("\n%-36s%s", "Warning :", "import thunk array is empty");
            continue;
        }

        DWORD ThunkSize = Is32Bit ? sizeof(DWORD) : sizeof(ULONGLONG);
        for (DWORD ThunkIndex = 0; TotalImports < MaxTotalImports; ThunkIndex++)
        {
            DWORD LookupEntryRva = 0;
            DWORD IatEntryRva    = 0;
            if (!PeAddDword(LookupThunkRva, ThunkIndex * ThunkSize, &LookupEntryRva) ||
                !PeAddDword(Descriptor->FirstThunk, ThunkIndex * ThunkSize, &IatEntryRva))
            {
                ShowMessages("\n%-36s%s", "Warning :", "import thunk RVA overflow");
                break;
            }

            ULONGLONG ThunkValue = 0;
            if (!PeReadThunkAtRva(Reader, LookupEntryRva, Is32Bit, &ThunkValue))
            {
                ShowMessages("\n%-36s%#x", "Warning, invalid thunk RVA :", LookupEntryRva);
                break;
            }

            if (ThunkValue == 0)
            {
                break;
            }

            ULONGLONG OrdinalFlag = Is32Bit ? IMAGE_ORDINAL_FLAG32 : IMAGE_ORDINAL_FLAG64;
            if ((ThunkValue & OrdinalFlag) != 0)
            {
                ShowMessages("\n    [%u] ordinal %u thunk RVA %#x", ThunkIndex, (UINT32)(ThunkValue & 0xffff), IatEntryRva);
                TotalImports++;
                continue;
            }

            if (ThunkValue > MAXDWORD)
            {
                ShowMessages("\n%-36s%#llx", "Warning, invalid import name RVA :", (UINT64)ThunkValue);
                TotalImports++;
                continue;
            }

            DWORD NameRva = (DWORD)ThunkValue;
            WORD  Hint    = 0;
            if (!PeReadWordAtRva(Reader, NameRva, &Hint))
            {
                ShowMessages("\n%-36s%#x", "Warning, invalid import hint RVA :", NameRva);
                TotalImports++;
                continue;
            }

            DWORD ImportNameRva = 0;
            if (!PeAddDword(NameRva, sizeof(WORD), &ImportNameRva))
            {
                ShowMessages("\n%-36s%#x", "Warning, invalid import name RVA :", NameRva);
                TotalImports++;
                continue;
            }

            CHAR                   ImportName[MaxImportNameLength + 1] = {0};
            PE_ASCII_STRING_STATUS ImportNameStatus                    = PeReadAsciiStringAtRva(Reader,
                                                                             ImportNameRva,
                                                                             MaxImportNameLength,
                                                                             ImportName,
                                                                             sizeof(ImportName));
            if (ImportNameStatus != PeAsciiStringOk)
            {
                ShowMessages("\n%-36s%#x, %s", "Warning, invalid import name RVA :", ImportNameRva, PeAsciiStatusName(ImportNameStatus));
                TotalImports++;
                continue;
            }

            ShowMessages("\n    [%u] hint %#x name %s thunk RVA %#x", ThunkIndex, Hint, ImportName, IatEntryRva);
            TotalImports++;
        }

        if (TotalImports >= MaxTotalImports)
        {
            TotalImportsCapped = TRUE;
            break;
        }
    }

    if (!FoundTerminator)
    {
        ShowMessages("\n%-36s%s", "Warning :", "import descriptor terminator not found before bounds or cap");
    }

    if (DescriptorCapped)
    {
        ShowMessages("\n%-36s%#x", "Warning, descriptor cap reached :", MaxImportDescriptors);
    }

    if (TotalImportsCapped)
    {
        ShowMessages("\n%-36s%#x", "Warning, import cap reached :", MaxTotalImports);
    }
}

/**
 * @brief Validates and retrieves a pointer to a PE export table array
 *
 * Computes the total byte size of the table as Count * EntrySize, verifies
 * that the multiplication does not overflow, and then resolves the table RVA
 * via PeGetPointerAtRva. When Count is zero *Pointer is set to NULL and TRUE
 * is returned without any range check.
 *
 * @param Reader    Pointer to an initialized PE_IMAGE_READER
 * @param TableRva  RVA of the export table array
 * @param Count     Number of entries in the table
 * @param EntrySize Size of each entry in bytes
 * @param Pointer   Output pointer set to the validated table location on success
 *
 * @return BOOLEAN TRUE on success, FALSE on overflow or if the RVA is not mapped
 */
static BOOLEAN
PeValidateExportTable(PPE_IMAGE_READER Reader, DWORD TableRva, DWORD Count, DWORD EntrySize, const BYTE ** Pointer)
{
    DWORD TableSize = 0;

    if (Count == 0)
    {
        *Pointer = NULL;
        return TRUE;
    }

    if (EntrySize != 0 && Count > MAXDWORD / EntrySize)
    {
        return FALSE;
    }

    TableSize = Count * EntrySize;
    return PeGetPointerAtRva(Reader, TableRva, TableSize, Pointer);
}

/**
 * @brief Prints one export entry including its ordinal, name, and function RVA or forwarder
 *
 * If FunctionRva falls within the export directory's RVA range the entry is
 * treated as a forwarder string and that string is read and displayed.
 * Otherwise the raw function RVA is printed.
 *
 * @param Reader          Pointer to an initialized PE_IMAGE_READER
 * @param ExportDirectory Pointer to the IMAGE_DIRECTORY_ENTRY_EXPORT data directory entry
 * @param Index           Zero-based display index of this export entry
 * @param Ordinal         Export ordinal (base-adjusted)
 * @param Name            Export name string, or an error description if not available
 * @param FunctionRva     RVA of the exported function or forwarder string
 */
static VOID
PeShowExportEntry(PPE_IMAGE_READER             Reader,
                  const IMAGE_DATA_DIRECTORY * ExportDirectory,
                  DWORD                        Index,
                  DWORD                        Ordinal,
                  const CHAR *                 Name,
                  DWORD                        FunctionRva)
{
    const DWORD MaxForwarderLength = 0x1000;

    if (PeRvaContainsRange(ExportDirectory->VirtualAddress, ExportDirectory->Size, FunctionRva, 1))
    {
        DWORD ForwarderRemaining = ExportDirectory->Size - (FunctionRva - ExportDirectory->VirtualAddress);
        DWORD ForwarderMaxLength = ForwarderRemaining < MaxForwarderLength ? ForwarderRemaining : MaxForwarderLength;

        CHAR                   Forwarder[MaxForwarderLength + 1] = {0};
        PE_ASCII_STRING_STATUS ForwarderStatus                   = PeReadAsciiStringAtRva(Reader,
                                                                        FunctionRva,
                                                                        ForwarderMaxLength,
                                                                        Forwarder,
                                                                        sizeof(Forwarder));
        if (ForwarderStatus == PeAsciiStringOk)
        {
            ShowMessages("\n[%u] ordinal %u name %s forwarder %s", Index, Ordinal, Name, Forwarder);
        }
        else
        {
            ShowMessages("\n[%u] ordinal %u name %s forwarder %s", Index, Ordinal, Name, PeAsciiStatusName(ForwarderStatus));
        }

        return;
    }

    ShowMessages("\n[%u] ordinal %u name %s RVA %#x", Index, Ordinal, Name, FunctionRva);
}

/**
 * @brief Prints the export directory including the export address, name, and ordinal tables
 *
 * Reads the IMAGE_EXPORT_DIRECTORY header, validates the three export tables
 * (AddressOfFunctions, AddressOfNames, AddressOfNameOrdinals), and iterates
 * all exported functions. Named exports are matched via the name-ordinal table;
 * unnamed exports are printed with their ordinal. Forwarder strings are handled
 * via PeShowExportEntry. Processing is capped at 0x2000 total exports.
 *
 * @param Reader          Pointer to an initialized PE_IMAGE_READER
 * @param ExportDirectory Pointer to the IMAGE_DIRECTORY_ENTRY_EXPORT data directory entry
 */
static VOID
PeShowExports(PPE_IMAGE_READER Reader, const IMAGE_DATA_DIRECTORY * ExportDirectory)
{
    const DWORD MaxExports          = 0x2000;
    const DWORD MaxDllNameLength    = 0x200;
    const DWORD MaxExportNameLength = 0x200;

    ShowMessages("\n\nExports\n-------");

    if (ExportDirectory->VirtualAddress == 0 || ExportDirectory->Size == 0)
    {
        ShowMessages("\n%-36s%s", "Export directory :", "empty");
        return;
    }

    if (!PeRvaContainsRange(ExportDirectory->VirtualAddress,
                            ExportDirectory->Size,
                            ExportDirectory->VirtualAddress,
                            sizeof(IMAGE_EXPORT_DIRECTORY)))
    {
        ShowMessages("\n%-36s%s", "Export directory :", "invalid bounds");
        return;
    }

    const IMAGE_EXPORT_DIRECTORY * Directory = NULL;
    if (!PeGetPointerAtRva(Reader,
                           ExportDirectory->VirtualAddress,
                           sizeof(IMAGE_EXPORT_DIRECTORY),
                           (const BYTE **)&Directory))
    {
        ShowMessages("\n%-36s%s", "Export directory :", "not mapped");
        return;
    }

    CHAR                   DllName[MaxDllNameLength + 1] = {0};
    PE_ASCII_STRING_STATUS DllNameStatus                 = PeReadAsciiStringAtRva(Reader,
                                                                  Directory->Name,
                                                                  MaxDllNameLength,
                                                                  DllName,
                                                                  sizeof(DllName));
    ShowMessages("\n%-36s%s", "DLL name :", DllNameStatus == PeAsciiStringOk ? DllName : PeAsciiStatusName(DllNameStatus));
    ShowMessages("\n%-36s%u", "Ordinal base :", Directory->Base);
    ShowMessages("\n%-36s%u", "Address table count :", Directory->NumberOfFunctions);
    ShowMessages("\n%-36s%u", "Name pointer count :", Directory->NumberOfNames);

    const BYTE * AddressTablePointer     = NULL;
    const BYTE * NamePointerTablePointer = NULL;
    const BYTE * OrdinalTablePointer     = NULL;

    BOOLEAN AddressTableValid     = PeValidateExportTable(Reader,
                                                      Directory->AddressOfFunctions,
                                                      Directory->NumberOfFunctions,
                                                      sizeof(DWORD),
                                                      &AddressTablePointer);
    BOOLEAN NamePointerTableValid = PeValidateExportTable(Reader,
                                                          Directory->AddressOfNames,
                                                          Directory->NumberOfNames,
                                                          sizeof(DWORD),
                                                          &NamePointerTablePointer);
    BOOLEAN OrdinalTableValid     = PeValidateExportTable(Reader,
                                                      Directory->AddressOfNameOrdinals,
                                                      Directory->NumberOfNames,
                                                      sizeof(WORD),
                                                      &OrdinalTablePointer);

    if (!AddressTableValid)
    {
        ShowMessages("\n%-36s%s", "Warning :", "export address table is not mapped");
        return;
    }

    if (!NamePointerTableValid)
    {
        ShowMessages("\n%-36s%s", "Warning :", "export name pointer table is not mapped");
    }

    if (!OrdinalTableValid)
    {
        ShowMessages("\n%-36s%s", "Warning :", "export ordinal table is not mapped");
    }

    DWORD   NamedCount  = Directory->NumberOfNames;
    BOOLEAN NamedCapped = FALSE;
    if (NamedCount > MaxExports)
    {
        NamedCount  = MaxExports;
        NamedCapped = TRUE;
    }

    DWORD   FunctionCount  = Directory->NumberOfFunctions;
    BOOLEAN FunctionCapped = FALSE;
    if (FunctionCount > MaxExports)
    {
        FunctionCount  = MaxExports;
        FunctionCapped = TRUE;
    }

    BYTE NamedAddressIndexes[MaxExports] = {0};

    if (NamePointerTableValid && OrdinalTableValid)
    {
        for (DWORD NameIndex = 0; NameIndex < NamedCount; NameIndex++)
        {
            DWORD NameRva      = 0;
            WORD  AddressIndex = 0;

            CopyMemory(&NameRva, NamePointerTablePointer + (NameIndex * sizeof(DWORD)), sizeof(NameRva));
            CopyMemory(&AddressIndex, OrdinalTablePointer + (NameIndex * sizeof(WORD)), sizeof(AddressIndex));

            if (AddressIndex >= Directory->NumberOfFunctions)
            {
                ShowMessages("\n%-36s%u", "Warning, invalid export ordinal index :", AddressIndex);
                continue;
            }

            DWORD FunctionRva = 0;
            CopyMemory(&FunctionRva, AddressTablePointer + (AddressIndex * sizeof(DWORD)), sizeof(FunctionRva));

            CHAR                   ExportName[MaxExportNameLength + 1] = {0};
            PE_ASCII_STRING_STATUS ExportNameStatus                    = PeReadAsciiStringAtRva(Reader,
                                                                             NameRva,
                                                                             MaxExportNameLength,
                                                                             ExportName,
                                                                             sizeof(ExportName));
            if (ExportNameStatus != PeAsciiStringOk)
            {
                ShowMessages("\n%-36s%#x, %s", "Warning, invalid export name RVA :", NameRva, PeAsciiStatusName(ExportNameStatus));
                continue;
            }

            DWORD DisplayOrdinal = 0;
            if (!PeAddDword(Directory->Base, AddressIndex, &DisplayOrdinal))
            {
                ShowMessages("\n%-36s%u", "Warning, invalid export ordinal index :", AddressIndex);
                continue;
            }

            if (AddressIndex < FunctionCount)
            {
                NamedAddressIndexes[AddressIndex] = TRUE;
            }

            PeShowExportEntry(Reader,
                              ExportDirectory,
                              NameIndex,
                              DisplayOrdinal,
                              ExportName,
                              FunctionRva);
        }
    }

    for (DWORD AddressIndex = 0; AddressIndex < FunctionCount; AddressIndex++)
    {
        if (NamedAddressIndexes[AddressIndex])
        {
            continue;
        }

        DWORD FunctionRva = 0;
        CopyMemory(&FunctionRva, AddressTablePointer + (AddressIndex * sizeof(DWORD)), sizeof(FunctionRva));

        DWORD DisplayOrdinal = 0;
        if (!PeAddDword(Directory->Base, AddressIndex, &DisplayOrdinal))
        {
            ShowMessages("\n%-36s%u", "Warning, invalid export ordinal index :", AddressIndex);
            continue;
        }

        PeShowExportEntry(Reader,
                          ExportDirectory,
                          AddressIndex,
                          DisplayOrdinal,
                          "<ordinal-only>",
                          FunctionRva);
    }

    if (NamedCapped)
    {
        ShowMessages("\n%-36s%#x", "Warning, named export cap reached :", MaxExports);
    }

    if (FunctionCapped)
    {
        ShowMessages("\n%-36s%#x", "Warning, export cap reached :", MaxExports);
    }
}

/**
 * @brief Locates the Rich header signature in a PE file
 *
 * The Rich header is an undocumented Microsoft structure embedded in PE files
 * that contains information about the tools and compilers used during the build process.
 * This function searches for the "Rich" signature string within the DOS stub area.
 *
 * @param DosHeader Pointer to the DOS header structure of the PE file
 * @param SearchSize Capped number of bytes to search
 * @param StartOffset Offset to start searching from
 * @param Key Output buffer to store the 4-byte XOR key found after "Rich" signature
 *
 * @note The Rich header is located between the DOS header and PE header
 * @note The XOR key is used to decode the actual Rich header entries
 **/
static INT
FindRichHeader(PIMAGE_DOS_HEADER DosHeader, DWORD SearchSize, DWORD StartOffset, CHAR Key[])
{
    if (DosHeader == NULL || Key == NULL)
    {
        return -1;
    }

    //
    // Get PE header offset - this defines our search boundary
    //
    LONG Offset = DosHeader->e_lfanew;

    if (SearchSize < sizeof(IMAGE_DOS_HEADER) ||
        DosHeader->e_magic != IMAGE_DOS_SIGNATURE ||
        Offset < (LONG)sizeof(IMAGE_DOS_HEADER) ||
        Offset < 8)
    {
        return -1;
    }

    CHAR * BaseAddr    = (CHAR *)DosHeader;
    DWORD  SearchLimit = (DWORD)Offset;
    if (SearchLimit > SearchSize)
    {
        SearchLimit = SearchSize;
    }

    if (StartOffset + 8 < StartOffset || StartOffset + 8 > SearchLimit)
    {
        return -1;
    }

    //
    // Search for "Rich" signature
    // We need 4 bytes for "Rich" and 4 bytes for the key before the PE header.
    //
    for (DWORD Offset = StartOffset; Offset + 8 <= SearchLimit; ++Offset)
    {
        //
        // Check for "Rich" signature (4 ASCII bytes)
        //
        if (BaseAddr[Offset] == 'R' &&
            BaseAddr[Offset + 1] == 'i' &&
            BaseAddr[Offset + 2] == 'c' &&
            BaseAddr[Offset + 3] == 'h')
        {
            //
            // Extract the 4-byte XOR key that immediately follows "Rich"
            //
            memcpy(Key, BaseAddr + Offset + 4, 4);

            //
            // Return the offset where "Rich" signature was found
            //
            return Offset;
        }
    }

    //
    // Rich header signature not found
    //
    return -1;
}

/**
 * @brief Decrypts Rich header data using XOR decryption and initializes header info
 *
 * The Rich header is encrypted using a simple XOR cipher with a 4-byte key.
 * This function decrypts the entire header in-place and populates the
 * PEFILE_RICH_HEADER_INFO structure with metadata about the decrypted data.
 *
 * @param RichHeaderPtr Pointer to the raw Rich header data to be decrypted
 * @param RichHeaderSize Size of the Rich header data in bytes
 * @param Key 4-byte XOR key used for decryption
 * @param PeFileRichHeaderInfo structure containing Rich header metadata and buffer information
 *
 * @note The Rich header uses a repeating 4-byte XOR key for encryption
 * @note After decryption, the header contains 16 bytes of metadata followed by 8-byte entries
 * @note Each entry represents one compilation tool (compiler, linker, assembler, etc.)
 *
 */
static BOOLEAN
FindRichEntries(CHAR *            RichHeaderPtr,
                INT               RichHeaderSize,
                CHAR              Key[],
                PRICH_HEADER_INFO PeFileRichHeaderInfo)
{
    if (RichHeaderPtr == NULL || Key == NULL || PeFileRichHeaderInfo == NULL ||
        RichHeaderSize < 16 || ((RichHeaderSize - 16) % 8) != 0)
    {
        return FALSE;
    }

    INT EntryCount = (RichHeaderSize - 16) / 8;

    if (EntryCount <= 0)
    {
        return FALSE;
    }

    //
    // Decrypt the entire Rich header using XOR with the 4-byte key
    //
    for (int i = 0; i < RichHeaderSize; i += 4)
    {
        //
        // Apply XOR decryption to each 4-byte block
        //
        for (int x = 0; x < 4; x++)
        {
            RichHeaderPtr[i + x] ^= Key[x];
        }
    }

    if (RichHeaderPtr[0] != 'D' || RichHeaderPtr[1] != 'a' ||
        RichHeaderPtr[2] != 'n' || RichHeaderPtr[3] != 'S')
    {
        return FALSE;
    }

    for (int i = 4; i < 16; i++)
    {
        if (RichHeaderPtr[i] != 0)
        {
            return FALSE;
        }
    }

    //
    // Initialize the Rich header info structure
    //
    PeFileRichHeaderInfo->Size        = RichHeaderSize;
    PeFileRichHeaderInfo->PtrToBuffer = RichHeaderPtr;

    //
    // Calculate number of entries: subtract 16-byte header, divide by 8 bytes per entry
    //
    PeFileRichHeaderInfo->Entries = EntryCount;

    return TRUE;
}

/**
 * @brief Parses decrypted Rich header data into structured entries
 *
 * After decryption, the Rich header contains a series of 8-byte entries, each describing
 * a compilation tool used during the build process. This function extracts and converts
 * the little-endian binary data into structured RICH_ENTRY objects.
 *
 * Rich header format after decryption:
 * - Bytes 0-15: Header metadata (DanS signature + padding)
 * - Bytes 16+: 8-byte entries (prodID:2, buildID:2, useCount:4)
 *
 * @param RichHeaderSize Size of the entire Rich header in bytes
 * @param RichHeaderPtr Pointer to the decrypted Rich header data
 * @param PeFileRichHeader structure containing the parsed Rich header entries
 *
 * @warning Assumes the Rich header has been properly decrypted first
 */
static BOOLEAN
SetRichEntries(INT RichHeaderSize, CHAR * RichHeaderPtr, PRICH_HEADER PeFileRichHeader)
{
    if (RichHeaderPtr == NULL || PeFileRichHeader == NULL || PeFileRichHeader->Entries == NULL ||
        RichHeaderSize < 16 || ((RichHeaderSize - 16) % 8) != 0)
    {
        return FALSE;
    }

    INT EntryCount = (RichHeaderSize - 16) / 8;

    if (EntryCount <= 0)
    {
        return FALSE;
    }

    //
    // Start at offset 16 to skip the header metadata, process 8-byte entries
    //
    for (int i = 16, EntryIndex = 0; i + 8 <= RichHeaderSize; i += 8, EntryIndex++)
    {
        //
        // Extract Product ID (bytes 2-3 of entry, little-endian)
        //
        WORD ProdID = ((UCHAR)RichHeaderPtr[i + 3] << 8) | (UCHAR)RichHeaderPtr[i + 2];

        //
        // Extract Build ID (bytes 0-1 of entry, little-endian)
        //
        WORD BuildID = ((UCHAR)RichHeaderPtr[i + 1] << 8) | (UCHAR)RichHeaderPtr[i];

        //
        // Extract Use Count (bytes 4-7 of entry, little-endian 32-bit)
        //
        DWORD UseCount = PeReadDwordFromBuffer((const BYTE *)RichHeaderPtr, i + 4);

        //
        // Store the parsed entry (adjust index: i/8 gives entry number, -2 for header offset)
        //
        PeFileRichHeader->Entries[EntryIndex] = {ProdID, BuildID, UseCount};
    }

    PeFileRichHeader->Entries[EntryCount] = {0x0000, 0x0000, 0x00000000};

    return TRUE;
}

/**
 * @brief Determines the size of the Rich header by finding the DanS signature
 *
 * The Rich header begins with the "DanS" signature (after decryption) and ends
 * with the "Rich" signature. This function works backwards from the "Rich" signature
 * to find the beginning and calculate the total size.
 *
 * Rich header structure:
 * [DanS signature] [Padding] [Tool Entries] [Rich signature] [XOR Key]
 *
 * @param Key 4-byte XOR key for decryption (extracted from after "Rich" signature)
 * @param Index Offset where "Rich" signature was found
 * @param DataPtr Pointer to the beginning of the PE file data
 *
 * @return INT Size of the Rich header in bytes, or 0 if DanS signature not found
 *
 */
static INT
DecryptRichHeader(CHAR Key[], INT Index, CHAR * DataPtr, INT DataSize)
{
    if (Key == NULL || DataPtr == NULL || DataSize < 16 || Index < 16 || Index + 4 > DataSize)
    {
        return 0;
    }

    //
    // Start searching backwards from just before the "Rich" signature
    //
    INT IndexPointer   = Index - 4;
    INT RichHeaderSize = 0;

    //
    // Search backwards for the DanS signature that marks the beginning
    //
    while (IndexPointer >= 0)
    {
        CHAR TmpChar[4];

        //
        // Read 4 bytes and decrypt them with the XOR key
        //
        memcpy(TmpChar, DataPtr + IndexPointer, 4);

        for (int i = 0; i < 4; i++)
        {
            TmpChar[i] ^= Key[i];
        }

        //
        // Move backwards and increment size counter
        //
        IndexPointer -= 4;
        RichHeaderSize += 4;

        //
        // Check for DanS signature after decryption.
        //
        if (TmpChar[0] == 'D' && TmpChar[1] == 'a' && TmpChar[2] == 'n' && TmpChar[3] == 'S')
        {
            return RichHeaderSize;
        }
    }

    return 0;
}

/**
 * @brief Show hex dump of sections of PE
 * @param Ptr
 * @param Size
 * @param SecAddress
 *
 * @return VOID
 */

static VOID
PeHexDump(CHAR * Ptr, SIZE_T Size, ULONGLONG SecAddress)
{
    SIZE_T i    = 1;
    INT    Temp = 0;

    if (Ptr == NULL || Size == 0)
    {
        return;
    }

    //
    // Buffer to store the character dump displayed at the
    // right side
    //
    CHAR Buf[18];
    ShowMessages("\n\n%llx: |", SecAddress);

    Buf[Temp]      = ' '; // initial space
    Buf[Temp + 16] = ' '; // final space
    Buf[Temp + 17] = 0;   // End of Buf
    Temp++;               // Temp = 1;

    for (; i <= Size; i++, Ptr++, Temp++)
    {
        Buf[Temp] = !iscntrl((*Ptr) & 0xff) ? (*Ptr) & 0xff : '.';
        ShowMessages("%-3.2x", (*Ptr) & 0xff);

        if (i % 16 == 0)
        {
            //
            // print the character dump to the right
            //
            ShowMessages("%s\n", Buf);
            if (i + 1 <= Size)
                ShowMessages("%llx: ", SecAddress += 16);
            Temp = 0;
        }
        if (i % 4 == 0)
            ShowMessages("| ");
    }
    if (i % 16 != 0)
    {
        Buf[Temp] = 0;
        for (; i % 16 != 0; i++)
            ShowMessages("%-3.2c", ' ');
        ShowMessages("%s\n", Buf);
    }
}

/**
 * @brief Show information about different sections of PE and the dump of sections
 * @param AddressOfFile
 * @param SectionToShow
 * @param Is32Bit
 *
 * @return BOOLEAN
 */
BOOLEAN
PeShowSectionInformationAndDump(const WCHAR * AddressOfFile,
                                const CHAR *  SectionToShow,
                                BOOLEAN       Is32Bit)
{
    RICH_HEADER_INFO             PeFileRichHeaderInfo {0};
    RICH_HEADER                  PeFileRichHeader {0};
    BOOLEAN                      Result = FALSE, RichFound = FALSE, SectionFound = FALSE;
    const DWORD                  MaxRichReadSize          = 1024 * 1024;
    const INT                    MaxRichHeaderSize        = 256 * 1024;
    const INT                    MaxRichHeaderEntries     = 0x4000;
    const DWORD                  MaxSectionScanBytes      = 16 * 1024 * 1024;
    const ULONGLONG              MaxTotalSectionScanBytes = 64 * 1024 * 1024;
    const SIZE_T                 MaxSectionDumpBytes      = 1024 * 1024;
    const ULONGLONG              MaxTotalSectionDumpBytes = 4 * 1024 * 1024;
    HANDLE                       MapObjectHandle = NULL, FileHandle = INVALID_HANDLE_VALUE; // File Mapping Object
    UINT32                       NumberOfSections;                                          // Number of sections
    LPVOID                       BaseAddr = NULL;                                           // Pointer to the base memory of mapped file
    PIMAGE_DOS_HEADER            DosHeader;                                                 // Pointer to DOS Header
    PIMAGE_NT_HEADERS32          NtHeader32 = NULL;                                         // Pointer to NT Header 32 bit
    PIMAGE_NT_HEADERS64          NtHeader64 = NULL;                                         // Pointer to NT Header 64 bit
    IMAGE_FILE_HEADER            Header;                                                    // Pointer to image file header of NT Header
    IMAGE_OPTIONAL_HEADER32      OpHeader32;                                                // Optional Header of PE files present in NT Header structure
    IMAGE_OPTIONAL_HEADER64      OpHeader64;                                                // Optional Header of PE files present in NT Header structure
    const IMAGE_SECTION_HEADER * SecHeader;                                                 // Section Header or Section Table Header
    PE_IMAGE_READER              Reader;
    IMAGE_DATA_DIRECTORY         EmptyDirectory = {0};
    LARGE_INTEGER                FileSize;
    CHAR                         Key[4];
    INT                          RichHeaderOffset          = -1;
    const CHAR *                 RichHeaderStatus          = "not found";
    BOOLEAN                      RichSearchCapped          = FALSE;
    ULONGLONG                    RemainingSectionScanBytes = MaxTotalSectionScanBytes;
    ULONGLONG                    RemainingSectionDumpBytes = MaxTotalSectionDumpBytes;
    time_t                       TimeDateStamp;

    //
    // Open the EXE File
    //
    FileHandle = CreateFileW(AddressOfFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        ShowMessages("err, could not open the file specified\n");
        return FALSE;
    }

    if (!GetFileSizeEx(FileHandle, &FileSize) || FileSize.QuadPart < (LONGLONG)sizeof(IMAGE_DOS_HEADER))
    {
        CloseHandle(FileHandle);
        return FALSE;
    }

    //
    // Mapping Given EXE file to Memory
    //
    MapObjectHandle = CreateFileMapping(FileHandle, NULL, PAGE_READONLY, 0, 0, NULL);

    if (MapObjectHandle == NULL)
    {
        CloseHandle(FileHandle);
        return FALSE;
    }

    BaseAddr = MapViewOfFile(MapObjectHandle, FILE_MAP_READ, 0, 0, 0);

    if (BaseAddr == NULL)
    {
        CloseHandle(MapObjectHandle);
        CloseHandle(FileHandle);
        return FALSE;
    }

    //
    // Get the DOS Header Base
    //
    DosHeader = (PIMAGE_DOS_HEADER)BaseAddr; // 0x04000000

    if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE ||
        DosHeader->e_lfanew < sizeof(IMAGE_DOS_HEADER) ||
        (LONGLONG)DosHeader->e_lfanew > FileSize.QuadPart ||
        (LONGLONG)DosHeader->e_lfanew + (LONGLONG)(Is32Bit ? sizeof(IMAGE_NT_HEADERS32) : sizeof(IMAGE_NT_HEADERS64)) > FileSize.QuadPart)
    {
        ShowMessages("\nGiven File is not a valid PE file\n");
        Result = FALSE;
        goto Finished;
    }

    if (!PeImageReaderInitialize((const BYTE *)BaseAddr, (SIZE_T)FileSize.QuadPart, &Reader))
    {
        ShowMessages("\nGiven File is not a valid PE file\n");
        Result = FALSE;
        goto Finished;
    }

    if (DosHeader->e_lfanew > 0 && (ULONGLONG)DosHeader->e_lfanew <= (ULONGLONG)MAXDWORD)
    {
        CHAR *        DataPtr      = NULL;
        DWORD         BytesRead    = 0;
        DWORD         RichReadSize = (DWORD)DosHeader->e_lfanew;
        DWORD         SearchOffset = 0;
        BOOLEAN       HadCandidate = FALSE;
        LARGE_INTEGER FileStart    = {0};

        if (RichReadSize > MaxRichReadSize)
        {
            RichReadSize     = MaxRichReadSize;
            RichHeaderStatus = "not found in first 1 MiB";
            RichSearchCapped = TRUE;
        }

        if (RichReadSize >= 8)
        {
            DataPtr = new (std::nothrow) CHAR[RichReadSize];
            if (DataPtr == NULL)
            {
                RichHeaderStatus = "allocation failed";
                goto SkipRichHeader;
            }

            if (!SetFilePointerEx(FileHandle, FileStart, NULL, FILE_BEGIN))
            {
                RichHeaderStatus = "read failed";
                delete[] DataPtr;
                goto SkipRichHeader;
            }

            BOOL Result = ReadFile(FileHandle, DataPtr, RichReadSize, &BytesRead, NULL);
            if (!Result || BytesRead != RichReadSize)
            {
                RichHeaderStatus = "read failed";
                delete[] DataPtr;
                goto SkipRichHeader;
            }

            for (;;)
            {
                CHAR * RichHeaderPtr = NULL;

                RichHeaderOffset = FindRichHeader(DosHeader, RichReadSize, SearchOffset, Key);
                if (RichHeaderOffset < 0)
                {
                    break;
                }

                HadCandidate     = TRUE;
                RichHeaderStatus = RichSearchCapped ? "malformed candidate in first 1 MiB" : "malformed";

                INT RichHeaderSize = DecryptRichHeader(Key, RichHeaderOffset, DataPtr, (INT)RichReadSize);
                INT IndexPointer   = RichHeaderOffset - RichHeaderSize;

                if (RichHeaderSize < 16 || RichHeaderSize > MaxRichHeaderSize || ((RichHeaderSize - 16) % 8) != 0 ||
                    (RichHeaderSize - 16) / 8 == 0 || (RichHeaderSize - 16) / 8 > MaxRichHeaderEntries ||
                    IndexPointer < 0 || RichHeaderSize > (INT)RichReadSize || IndexPointer > (INT)RichReadSize - RichHeaderSize)
                {
                    SearchOffset = (DWORD)RichHeaderOffset + 1;
                    continue;
                }

                RichHeaderPtr = new (std::nothrow) CHAR[RichHeaderSize];
                if (RichHeaderPtr == NULL)
                {
                    RichHeaderStatus = "allocation failed";
                    break;
                }

                memcpy(RichHeaderPtr, DataPtr + IndexPointer, RichHeaderSize);

                if (!FindRichEntries(RichHeaderPtr, RichHeaderSize, Key, &PeFileRichHeaderInfo))
                {
                    delete[] RichHeaderPtr;
                    SearchOffset = (DWORD)RichHeaderOffset + 1;
                    continue;
                }

                RichHeaderPtr = NULL;

                if (PeFileRichHeaderInfo.Entries > MaxRichHeaderEntries || PeFileRichHeaderInfo.Entries >= INT_MAX / (INT)sizeof(RICH_HEADER_ENTRY) - 1)
                {
                    delete[] PeFileRichHeaderInfo.PtrToBuffer;
                    PeFileRichHeaderInfo = {0};
                    SearchOffset         = (DWORD)RichHeaderOffset + 1;
                    continue;
                }

                PeFileRichHeader.Entries = new (std::nothrow) RICH_HEADER_ENTRY[PeFileRichHeaderInfo.Entries + 1];
                if (PeFileRichHeader.Entries == NULL)
                {
                    RichHeaderStatus = "allocation failed";
                    delete[] PeFileRichHeaderInfo.PtrToBuffer;
                    PeFileRichHeaderInfo = {0};
                    break;
                }

                if (!SetRichEntries(RichHeaderSize, PeFileRichHeaderInfo.PtrToBuffer, &PeFileRichHeader))
                {
                    delete[] PeFileRichHeaderInfo.PtrToBuffer;
                    PeFileRichHeaderInfo = {0};
                    delete[] PeFileRichHeader.Entries;
                    PeFileRichHeader.Entries = NULL;
                    SearchOffset             = (DWORD)RichHeaderOffset + 1;
                    continue;
                }

                RichFound = TRUE;
                break;
            }

            if (!RichFound && !HadCandidate && (DWORD)DosHeader->e_lfanew <= MaxRichReadSize)
            {
                RichHeaderStatus = "not found";
            }

            delete[] DataPtr;
        }
    }

SkipRichHeader:

    //
    // Check for Valid DOS file
    //
    if (DosHeader->e_magic == IMAGE_DOS_SIGNATURE)
    {
        //
        // Dump the Dos Header info
        //
        ShowMessages("\nValid Dos Exe File\n------------------\n");
        ShowMessages("\nDumping DOS Header Info....\n---------------------------");
        ShowMessages("\n%-36s%s ",
                     "Magic number : ",
                     DosHeader->e_magic == 0x5a4d ? "MZ" : "-");
        ShowMessages("\n%-36s%#x", "Bytes on last page of file :", DosHeader->e_cblp);
        ShowMessages("\n%-36s%#x", "Pages in file : ", DosHeader->e_cp);
        ShowMessages("\n%-36s%#x", "Relocation : ", DosHeader->e_crlc);
        ShowMessages("\n%-36s%#x",
                     "Size of header in paragraphs : ",
                     DosHeader->e_cparhdr);
        ShowMessages("\n%-36s%#x",
                     "Minimum extra paragraphs needed : ",
                     DosHeader->e_minalloc);
        ShowMessages("\n%-36s%#x",
                     "Maximum extra paragraphs needed : ",
                     DosHeader->e_maxalloc);
        ShowMessages("\n%-36s%#x", "Initial (relative) SS value : ", DosHeader->e_ss);
        ShowMessages("\n%-36s%#x", "Initial SP value : ", DosHeader->e_sp);
        ShowMessages("\n%-36s%#x", "Checksum : ", DosHeader->e_csum);
        ShowMessages("\n%-36s%#x", "Initial IP value : ", DosHeader->e_ip);
        ShowMessages("\n%-36s%#x", "Initial (relative) CS value : ", DosHeader->e_cs);
        ShowMessages("\n%-36s%#x",
                     "File address of relocation table : ",
                     DosHeader->e_lfarlc);
        ShowMessages("\n%-36s%#x", "Overlay number : ", DosHeader->e_ovno);
        ShowMessages("\n%-36s%#x", "OEM identifier : ", DosHeader->e_oemid);
        ShowMessages("\n%-36s%#x",
                     "OEM information(e_oemid specific) :",
                     DosHeader->e_oeminfo);
        ShowMessages("\n%-36s%#x", "RVA address of PE header : ", DosHeader->e_lfanew);
        ShowMessages("\n==============================================================="
                     "================\n");
    }
    else
    {
        ShowMessages("\nGiven File is not a valid DOS file\n");
        Result = FALSE;
        goto Finished;
    }

    if (RichFound)
    {
        ShowMessages("\n===============================================================================\n");
        ShowMessages("                              RICH HEADER                                     \n");
        ShowMessages("===============================================================================\n");
        ShowMessages("Entries: %d\n\n", PeFileRichHeaderInfo.Entries);
        ShowMessages("%-10s %-10s %-10s\n", "Build ID", "Prod ID", "Use Count");
        ShowMessages("---------------------------------------\n");

        for (int i = 0; i < PeFileRichHeaderInfo.Entries; i++)
        {
            ShowMessages("0x%08X 0x%08X %10u\n",
                         PeFileRichHeader.Entries[i].BuildID,
                         PeFileRichHeader.Entries[i].ProdID,
                         PeFileRichHeader.Entries[i].UseCount);
        }

        ShowMessages("==============Rich Header End ==================\n");
    }
    else
    {
        ShowMessages("=========== Rich Header Not Shown (%s) ===========\n", RichHeaderStatus);
    }

    //
    // Offset of NT Header is found at 0x3c location in DOS header specified by
    // e_lfanew
    // Get the Base of NT Header(PE Header) 	= DosHeader + RVA address of PE
    // header
    //
    if (Is32Bit)
    {
        NtHeader32 = (PIMAGE_NT_HEADERS32)((UINT64)(DosHeader) + (DosHeader->e_lfanew));
    }
    else
    {
        NtHeader64 = (PIMAGE_NT_HEADERS64)((UINT64)(DosHeader) + (DosHeader->e_lfanew));
    }

    //
    // Identify for valid PE file
    //
    if (Is32Bit && NtHeader32->Signature == IMAGE_NT_SIGNATURE)
    {
        ShowMessages("\nValid PE32 file \n-------------\n");
    }
    else if (!Is32Bit && NtHeader64->Signature == IMAGE_NT_SIGNATURE)
    {
        ShowMessages("\nValid PE64 file \n-------------\n");
    }
    else
    {
        ShowMessages("err, the specified file is not a valid PE file");
        Result = FALSE;
        goto Finished;
    }

    //
    // Dump NT Header Info....
    //
    ShowMessages("\nDumping COFF/PE Header "
                 "Info....\n--------------------------------");
    ShowMessages("\n%-36s%s", "Signature :", "PE");

    //
    // Get the IMAGE FILE HEADER Structure
    //
    if (Is32Bit)
    {
        Header = NtHeader32->FileHeader;
    }
    else
    {
        Header = NtHeader64->FileHeader;
    }

    //
    // Determine Machine Architecture
    //
    ShowMessages("\n%-36s", "Machine Architecture :");

    //
    // Only few are determined (for remaining refer
    // to the above specification)
    //
    switch (Header.Machine)
    {
    case 0x0:
        ShowMessages("All ");
        break;
    case 0x14d:
        ShowMessages("Intel i860");
        break;
    case 0x14c:
        ShowMessages("Intel i386, i486, i586");
        break;
    case 0x200:
        ShowMessages("Intel Itanium processor");
        break;
    case 0x8664:
        ShowMessages("AMD x64");
        break;
    case 0x162:
        ShowMessages("MIPS R3000");
        break;
    case 0x166:
        ShowMessages("MIPS R4000");
        break;
    case 0x183:
        ShowMessages("DEC Alpha AXP");
        break;
    default:
        ShowMessages("Not Found");
        break;
    }

    //
    // Determine the characteristics of the given file
    //
    ShowMessages("\n%-36s", "Characteristics : ");
    if ((Header.Characteristics & 0x0002) == 0x0002)
        ShowMessages("Executable Image, ");
    if ((Header.Characteristics & 0x0020) == 0x0020)
        ShowMessages("Application can address > 2GB, ");
    if ((Header.Characteristics & 0x1000) == 0x1000)
        ShowMessages("System file (Kernel Mode Driver(I think)), ");
    if ((Header.Characteristics & 0x2000) == 0x2000)
        ShowMessages("Dll file, ");
    if ((Header.Characteristics & 0x4000) == 0x4000)
        ShowMessages("Application runs only in Uniprocessor, ");

    //
    // Determine Time Stamp
    //
    TimeDateStamp = Header.TimeDateStamp;
    ShowMessages("\n%-36s%s",
                 "Time Stamp :",
                 ctime(&TimeDateStamp));

    //
    // Determine number of sections
    //
    ShowMessages("%-36s%d", "No.sections(size) :", Header.NumberOfSections);
    ShowMessages("\n%-36s%d", "No.entries in symbol table :", Header.NumberOfSymbols);
    ShowMessages("\n%-36s%d",
                 "Size of optional header :",
                 Header.SizeOfOptionalHeader);
    ShowMessages("\n%-36s%#x", "Raw machine value :", Header.Machine);
    ShowMessages("\n%-36s%#x", "Pointer to symbol table :", Header.PointerToSymbolTable);
    ShowMessages("\n%-36s%#x", "Raw characteristics value :", Header.Characteristics);

    ShowMessages("\n\nDumping PE Optional Header "
                 "Info....\n-----------------------------------");

    if (Is32Bit)
    {
        //
        // Info about Optional Header
        //
        OpHeader32 = NtHeader32->OptionalHeader;
        ShowMessages("\n\nInfo of optional Header\n-----------------------");
        ShowMessages("\n%-36s%#x",
                     "Address of Entry Point : ",
                     OpHeader32.AddressOfEntryPoint);
        ShowMessages("\n%-36s%#x", "Raw optional header magic :", OpHeader32.Magic);
        PeShowEntrypointFileOffset(&Reader, OpHeader32.AddressOfEntryPoint);
        ShowMessages("\n%-36s%#llx", "Base Address of the Image : ", OpHeader32.ImageBase);
        ShowMessages("\n%-36s%s", "SubSystem type : ", PeGetSubsystemName(OpHeader32.Subsystem));
        ShowMessages("\n%-36s%s", "Given file is a : ", OpHeader32.Magic == 0x20b ? "PE32+(64)" : "PE32");
        ShowMessages("\n%-36s%#x", "File Alignment :", OpHeader32.FileAlignment);
        ShowMessages("\n%-36s%#x", "Size of Image :", OpHeader32.SizeOfImage);
        ShowMessages("\n%-36s%#x", "Size of Headers :", OpHeader32.SizeOfHeaders);
        PeShowChecksum(&Reader,
                       OpHeader32.CheckSum,
                       (SIZE_T)(Reader.NtHeaders - Reader.ImageBase) + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + FIELD_OFFSET(IMAGE_OPTIONAL_HEADER32, CheckSum));
        ShowMessages("\n%-36s%#x", "Raw DLL characteristics value :", OpHeader32.DllCharacteristics);
        ShowMessages("\n%-36s", "DLL characteristics flags :");
        PeShowDllCharacteristics(OpHeader32.DllCharacteristics);
        ShowMessages("\n%-36s%d", "Size of code segment(.text) : ", OpHeader32.SizeOfCode);
        ShowMessages("\n%-36s%#x",
                     "Base address of code segment(RVA) :",
                     OpHeader32.BaseOfCode);
        ShowMessages("\n%-36s%d",
                     "Size of Initialized data : ",
                     OpHeader32.SizeOfInitializedData);

        ShowMessages("\n%-36s%#x",
                     "Base address of data segment(RVA) :",
                     OpHeader32.BaseOfData);

        ShowMessages("\n%-36s%#x", "Section Alignment :", OpHeader32.SectionAlignment);
        ShowMessages("\n%-36s%d", "Major Linker Version : ", OpHeader32.MajorLinkerVersion);
        ShowMessages("\n%-36s%d", "Minor Linker Version : ", OpHeader32.MinorLinkerVersion);
        PeShowDataDirectories(&Reader, OpHeader32.DataDirectory, OpHeader32.NumberOfRvaAndSizes);
        PeShowCertificateTable(&Reader,
                               OpHeader32.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_SECURITY ? &OpHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY] : &EmptyDirectory);
        PeShowBaseRelocations(&Reader,
                              OpHeader32.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_BASERELOC ? &OpHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC] : &EmptyDirectory);
        PeShowBoundImports(&Reader,
                           OpHeader32.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT ? &OpHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT] : &EmptyDirectory);
        PeShowResources(&Reader,
                        OpHeader32.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_RESOURCE ? &OpHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE] : &EmptyDirectory);
        PeShowExceptions(&Reader,
                         OpHeader32.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_EXCEPTION ? &OpHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION] : &EmptyDirectory,
                         Header.Machine,
                         TRUE);
        PeShowDelayImports(&Reader,
                           OpHeader32.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT ? &OpHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT] : &EmptyDirectory,
                           TRUE,
                           OpHeader32.ImageBase);
        PeShowClrRuntime(&Reader,
                         OpHeader32.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR ? &OpHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR] : &EmptyDirectory);
        PeShowTls(&Reader,
                  OpHeader32.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_TLS ? &OpHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS] : &EmptyDirectory,
                  TRUE,
                  OpHeader32.ImageBase);
        PeShowDebug(&Reader,
                    OpHeader32.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_DEBUG ? &OpHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG] : &EmptyDirectory);
        PeShowLoadConfig(&Reader,
                         OpHeader32.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG ? &OpHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG] : &EmptyDirectory,
                         TRUE);
        PeShowImports(&Reader,
                      OpHeader32.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_IMPORT ? &OpHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT] : &EmptyDirectory,
                      TRUE);
        PeShowExports(&Reader,
                      OpHeader32.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_EXPORT ? &OpHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT] : &EmptyDirectory);
    }
    else
    {
        //
        // Info about Optional Header
        //
        OpHeader64 = NtHeader64->OptionalHeader;
        ShowMessages("\n\nInfo of optional Header\n-----------------------");
        ShowMessages("\n%-36s%#x",
                     "Address of Entry Point : ",
                     OpHeader64.AddressOfEntryPoint);
        ShowMessages("\n%-36s%#x", "Raw optional header magic :", OpHeader64.Magic);
        PeShowEntrypointFileOffset(&Reader, OpHeader64.AddressOfEntryPoint);
        ShowMessages("\n%-36s%#llx", "Base Address of the Image : ", OpHeader64.ImageBase);
        ShowMessages("\n%-36s%s", "SubSystem type : ", PeGetSubsystemName(OpHeader64.Subsystem));
        ShowMessages("\n%-36s%s", "Given file is a : ", OpHeader64.Magic == 0x20b ? "PE32+(64)" : "PE32");
        ShowMessages("\n%-36s%#x", "File Alignment :", OpHeader64.FileAlignment);
        ShowMessages("\n%-36s%#x", "Size of Image :", OpHeader64.SizeOfImage);
        ShowMessages("\n%-36s%#x", "Size of Headers :", OpHeader64.SizeOfHeaders);
        PeShowChecksum(&Reader,
                       OpHeader64.CheckSum,
                       (SIZE_T)(Reader.NtHeaders - Reader.ImageBase) + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + FIELD_OFFSET(IMAGE_OPTIONAL_HEADER64, CheckSum));
        ShowMessages("\n%-36s%#x", "Raw DLL characteristics value :", OpHeader64.DllCharacteristics);
        ShowMessages("\n%-36s", "DLL characteristics flags :");
        PeShowDllCharacteristics(OpHeader64.DllCharacteristics);
        ShowMessages("\n%-36s%d", "Size of code segment(.text) : ", OpHeader64.SizeOfCode);
        ShowMessages("\n%-36s%#x",
                     "Base address of code segment(RVA) :",
                     OpHeader64.BaseOfCode);
        ShowMessages("\n%-36s%d",
                     "Size of Initialized data : ",
                     OpHeader64.SizeOfInitializedData);

        ShowMessages("\n%-36s%#x", "Section Alignment :", OpHeader64.SectionAlignment);
        ShowMessages("\n%-36s%d", "Major Linker Version : ", OpHeader64.MajorLinkerVersion);
        ShowMessages("\n%-36s%d", "Minor Linker Version : ", OpHeader64.MinorLinkerVersion);
        PeShowDataDirectories(&Reader, OpHeader64.DataDirectory, OpHeader64.NumberOfRvaAndSizes);
        PeShowCertificateTable(&Reader,
                               OpHeader64.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_SECURITY ? &OpHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY] : &EmptyDirectory);
        PeShowBaseRelocations(&Reader,
                              OpHeader64.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_BASERELOC ? &OpHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC] : &EmptyDirectory);
        PeShowBoundImports(&Reader,
                           OpHeader64.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT ? &OpHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT] : &EmptyDirectory);
        PeShowResources(&Reader,
                        OpHeader64.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_RESOURCE ? &OpHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE] : &EmptyDirectory);
        PeShowExceptions(&Reader,
                         OpHeader64.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_EXCEPTION ? &OpHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION] : &EmptyDirectory,
                         Header.Machine,
                         FALSE);
        PeShowDelayImports(&Reader,
                           OpHeader64.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT ? &OpHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT] : &EmptyDirectory,
                           FALSE,
                           OpHeader64.ImageBase);
        PeShowClrRuntime(&Reader,
                         OpHeader64.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR ? &OpHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR] : &EmptyDirectory);
        PeShowTls(&Reader,
                  OpHeader64.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_TLS ? &OpHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS] : &EmptyDirectory,
                  FALSE,
                  OpHeader64.ImageBase);
        PeShowDebug(&Reader,
                    OpHeader64.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_DEBUG ? &OpHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG] : &EmptyDirectory);
        PeShowLoadConfig(&Reader,
                         OpHeader64.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG ? &OpHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG] : &EmptyDirectory,
                         FALSE);
        PeShowImports(&Reader,
                      OpHeader64.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_IMPORT ? &OpHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT] : &EmptyDirectory,
                      FALSE);
        PeShowExports(&Reader,
                      OpHeader64.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_EXPORT ? &OpHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT] : &EmptyDirectory);
    }

    ShowMessages("\n\nDumping Sections Header "
                 "Info....\n--------------------------------");

    //
    // Retrieve a pointer to First Section Header(or Section Table Entry)
    //
    SecHeader        = Reader.SectionHeaders;
    NumberOfSections = Reader.FileHeader->NumberOfSections;

    for (UINT32 i = 0; i < NumberOfSections; i++, SecHeader++)
    {
        CHAR         SectionName[IMAGE_SIZEOF_SHORT_NAME + 1];
        const BYTE * Pointer       = NULL;
        const CHAR * RawDataBounds = "empty";

        PeImageReaderGetSectionName(SecHeader, SectionName, sizeof(SectionName));

        if (SecHeader->SizeOfRawData != 0)
        {
            RawDataBounds = PeImageReaderGetPointerAtOffset(&Reader,
                                                            SecHeader->PointerToRawData,
                                                            SecHeader->SizeOfRawData,
                                                            &Pointer)
                                ? "valid"
                                : "invalid";
        }

        ShowMessages("\n\nSection Info (%d of %d)", i + 1, NumberOfSections);

        ShowMessages("\n---------------------");
        ShowMessages("\n%-36s%s", "Section Header name : ", SectionName);
        ShowMessages("\n%-36s%#x",
                     "ActualSize of code or data : ",
                     SecHeader->Misc.VirtualSize);
        ShowMessages("\n%-36s%#x", "Virtual Address(RVA) :", SecHeader->VirtualAddress);
        ShowMessages("\n%-36s%#x",
                     "Size of raw data (rounded to FA) : ",
                     SecHeader->SizeOfRawData);
        ShowMessages("\n%-36s%#x",
                     "Pointer to Raw Data : ",
                     SecHeader->PointerToRawData);
        ShowMessages("\n%-36s%s", "Raw data bounds :", RawDataBounds);
        if (Pointer != NULL && SecHeader->SizeOfRawData != 0)
        {
            if (SecHeader->SizeOfRawData > MaxSectionScanBytes)
            {
                ShowMessages("\n%-36s%s", "Raw data entropy :", "skipped, section is larger than local cap");
                ShowMessages("\n%-36s%s", "Raw data FNV-1a64 :", "skipped, section is larger than local cap");
            }
            else if ((ULONGLONG)SecHeader->SizeOfRawData > RemainingSectionScanBytes)
            {
                RemainingSectionScanBytes = 0;
                ShowMessages("\n%-36s%s", "Raw data entropy :", "skipped, global scan budget exhausted");
                ShowMessages("\n%-36s%s", "Raw data FNV-1a64 :", "skipped, global scan budget exhausted");
            }
            else
            {
                ShowMessages("\n%-36s%.4f", "Raw data entropy :", PeCalculateEntropy(Pointer, SecHeader->SizeOfRawData));
                ShowMessages("\n%-36s%#llx", "Raw data FNV-1a64 :", PeFnv1a64(Pointer, SecHeader->SizeOfRawData));
                RemainingSectionScanBytes -= SecHeader->SizeOfRawData;
            }
        }
        if (SecHeader->SizeOfRawData != 0 && SecHeader->Misc.VirtualSize > SecHeader->SizeOfRawData)
        {
            ShowMessages("\n%-36s%s", "Warning :", "virtual size is larger than raw data");
        }
        ShowMessages("\n%-36s%#x",
                     "Pointer to Relocations : ",
                     SecHeader->PointerToRelocations);
        ShowMessages("\n%-36s%#x",
                     "Pointer to Line numbers : ",
                     SecHeader->PointerToLinenumbers);
        ShowMessages("\n%-36s%#x",
                     "Number of relocations : ",
                     SecHeader->NumberOfRelocations);
        ShowMessages("\n%-36s%#x",
                     "Number of line numbers : ",
                     SecHeader->NumberOfLinenumbers);
        ShowMessages("\n%-36s%s", "Characteristics : ", "Contains ");
        if ((SecHeader->Characteristics & 0x20) == 0x20)
            ShowMessages("executable code, ");
        if ((SecHeader->Characteristics & 0x40) == 0x40)
            ShowMessages("initialized data, ");
        if ((SecHeader->Characteristics & 0x80) == 0x80)
            ShowMessages("uninitialized data, ");
        if ((SecHeader->Characteristics & 0x200) == 0x200)
            ShowMessages("comments and linker commands, ");
        if ((SecHeader->Characteristics & 0x10000000) == 0x10000000)
            ShowMessages("shareable data(via DLLs), ");
        if ((SecHeader->Characteristics & 0x40000000) == 0x40000000)
            ShowMessages("Readable, ");
        if ((SecHeader->Characteristics & 0x80000000) == 0x80000000)
            ShowMessages("Writable, ");

        //
        // show the hex dump if the user needs it
        //
        if (SectionToShow != NULL)
        {
            if (!_strcmpi(SectionToShow, SectionName))
            {
                SectionFound = TRUE;

                if (SecHeader->SizeOfRawData != 0)
                {
                    SIZE_T  DumpSize              = SecHeader->SizeOfRawData;
                    BOOLEAN TruncatedBySectionCap = FALSE;
                    BOOLEAN TruncatedByGlobalCap  = FALSE;

                    if (Pointer == NULL)
                    {
                        ShowMessages("\nerr, invalid section raw data\n");
                        continue;
                    }

                    if (RemainingSectionDumpBytes == 0)
                    {
                        ShowMessages("\n%-36s%s", "Warning, section dump skipped :", "global dump budget exhausted");
                        continue;
                    }

                    if (DumpSize > MaxSectionDumpBytes)
                    {
                        DumpSize              = MaxSectionDumpBytes;
                        TruncatedBySectionCap = TRUE;
                    }

                    if ((ULONGLONG)DumpSize > RemainingSectionDumpBytes)
                    {
                        DumpSize             = (SIZE_T)RemainingSectionDumpBytes;
                        TruncatedByGlobalCap = TRUE;
                    }

                    if (TruncatedBySectionCap)
                    {
                        ShowMessages("\n%-36s%#llx of %#x bytes",
                                     "Warning, section dump truncated by section cap :",
                                     (UINT64)DumpSize,
                                     SecHeader->SizeOfRawData);
                    }
                    if (TruncatedByGlobalCap)
                    {
                        ShowMessages("\n%-36s%#llx of %#x bytes",
                                     "Warning, section dump truncated by global cap :",
                                     (UINT64)DumpSize,
                                     SecHeader->SizeOfRawData);
                    }

                    if (Is32Bit)
                    {
                        PeHexDump((CHAR *)Pointer,
                                  DumpSize,
                                  (ULONGLONG)OpHeader32.ImageBase + SecHeader->VirtualAddress);
                    }
                    else
                    {
                        PeHexDump((CHAR *)Pointer,
                                  DumpSize,
                                  OpHeader64.ImageBase + SecHeader->VirtualAddress);
                    }

                    RemainingSectionDumpBytes -= DumpSize;
                }
            }
        }
    }

    PeShowOverlayAndWarnings(&Reader,
                             Header.Machine,
                             Is32Bit ? OpHeader32.Magic : OpHeader64.Magic,
                             Is32Bit ? OpHeader32.AddressOfEntryPoint : OpHeader64.AddressOfEntryPoint,
                             Is32Bit ? OpHeader32.SizeOfImage : OpHeader64.SizeOfImage,
                             Is32Bit ? OpHeader32.SizeOfHeaders : OpHeader64.SizeOfHeaders,
                             Is32Bit ? OpHeader32.DataDirectory : OpHeader64.DataDirectory,
                             Is32Bit ? OpHeader32.NumberOfRvaAndSizes : OpHeader64.NumberOfRvaAndSizes);

    if (SectionToShow != NULL && !SectionFound)
    {
        ShowMessages("\nerr, section '%s' was not found\n", SectionToShow);
    }

    ShowMessages("\n==============================================================="
                 "================\n");

    //
    // Set result to true
    //
    Result = TRUE;

Finished:
    //
    // Unmap and close the handles
    //
    if (PeFileRichHeaderInfo.PtrToBuffer != NULL)
    {
        delete[] PeFileRichHeaderInfo.PtrToBuffer;
    }

    if (PeFileRichHeader.Entries != NULL)
    {
        delete[] PeFileRichHeader.Entries;
    }

    if (BaseAddr != NULL)
    {
        UnmapViewOfFile(BaseAddr);
    }

    if (MapObjectHandle != NULL)
    {
        CloseHandle(MapObjectHandle);
    }

    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(FileHandle);
    }

    return Result;
}

/**
 * @brief Detect whether PE is a 32-bit PE or 64-bit PE
 * @param AddressOfFile
 * @param Is32Bit
 *
 * @return BOOLEAN
 */
BOOLEAN
PeIsPE32BitOr64Bit(const WCHAR * AddressOfFile, PBOOLEAN Is32Bit)
{
    BOOLEAN             Result = FALSE;
    HANDLE              MapObjectHandle, FileHandle; // File Mapping Object
    LPVOID              BaseAddr;                    // Pointer to the base memory of mapped file
    PIMAGE_DOS_HEADER   DosHeader;                   // Pointer to DOS Header
    PIMAGE_NT_HEADERS32 NtHeader32 = NULL;           // Pointer to NT Header 32 bit
    PE_IMAGE_READER     Reader;
    LARGE_INTEGER       FileSize;

    //
    // Open the EXE File
    //
    FileHandle = CreateFileW(AddressOfFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        ShowMessages("err, unable to read the file (%x)\n", GetLastError());
        return FALSE;
    };

    if (!GetFileSizeEx(FileHandle, &FileSize) || FileSize.QuadPart < (LONGLONG)sizeof(IMAGE_DOS_HEADER))
    {
        CloseHandle(FileHandle);
        return FALSE;
    }

    //
    // Mapping Given EXE file to Memory
    //
    MapObjectHandle =
        CreateFileMapping(FileHandle, NULL, PAGE_READONLY, 0, 0, NULL);

    if (MapObjectHandle == NULL)
    {
        CloseHandle(FileHandle);

        ShowMessages("err, unable to create file mappings (%x)\n", GetLastError());
        return FALSE;
    }

    BaseAddr = MapViewOfFile(MapObjectHandle, FILE_MAP_READ, 0, 0, 0);

    if (BaseAddr == NULL)
    {
        CloseHandle(MapObjectHandle);
        CloseHandle(FileHandle);

        ShowMessages("err, unable to create map view of file (%x)\n", GetLastError());
        return FALSE;
    }

    //
    // Get the DOS Header Base
    //
    DosHeader = (PIMAGE_DOS_HEADER)BaseAddr; // 0x04000000

    //
    // Check for Valid DOS file
    //
    if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        Result = FALSE;

        ShowMessages("err, the selected file is not in a valid PE format\n");
        goto Finished;
    }

    if (DosHeader->e_lfanew < sizeof(IMAGE_DOS_HEADER) ||
        (LONGLONG)DosHeader->e_lfanew + (LONGLONG)sizeof(IMAGE_NT_HEADERS32) > FileSize.QuadPart)
    {
        Result = FALSE;

        ShowMessages("err, invalid image NT header offset\n");
        goto Finished;
    }

    //
    // Offset of NT Header is found at 0x3c location in DOS header specified by
    // e_lfanew
    // Get the Base of NT Header(PE Header) 	= DosHeader + RVA address of PE
    // header
    //

    NtHeader32 = (PIMAGE_NT_HEADERS32)((UINT64)(DosHeader) + (DosHeader->e_lfanew));

    //
    // Identify for valid PE file
    //
    if (NtHeader32->Signature != IMAGE_NT_SIGNATURE)
    {
        Result = FALSE;

        ShowMessages("err, invalid image NT signature\n");
        goto Finished;
    }

    if (!PeImageReaderInitialize((const BYTE *)BaseAddr, (SIZE_T)FileSize.QuadPart, &Reader))
    {
        Result = FALSE;

        ShowMessages("err, the selected file is not in a valid PE format\n");
        goto Finished;
    }

    //
    // Only few are determined (for remaining refer
    // to the above specification)
    //
    switch (Reader.FileHeader->Machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        *Is32Bit = TRUE;
        Result   = TRUE;
        goto Finished;
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        *Is32Bit = FALSE;
        Result   = TRUE;
        goto Finished;
        break;
    default:
        ShowMessages("err, PE file is not i386 or AMD64; thus, it's not supported "
                     "in HyperDbg\n");
        Result = FALSE;
        goto Finished;
        break;
    }

Finished:
    //
    // Unmap and close the handles
    //
    UnmapViewOfFile(BaseAddr);
    CloseHandle(MapObjectHandle);
    CloseHandle(FileHandle);

    return Result;
}

/**
 * @brief Get the syscall number of a given Nt function
 * @param NtFunctionName
 *
 * @return UINT32
 */
UINT32
PeGetSyscallNumber(LPCSTR NtFunctionName)
{
    HMODULE DllHandle = NULL;

    //
    // Load ntdll.dll to get the address of the Nt function
    //
    DllHandle = LoadLibraryA("ntdll.dll");

    if (!DllHandle)
    {
        ShowMessages("err, failed to load ntdll.dll\n");
        return NULL;
    }

    //
    // Choose any Nt function
    //
    VOID * TargetFunc = GetProcAddress(DllHandle, NtFunctionName);

    if (!TargetFunc)
    {
        //
        // If we failed to get the address of the Nt function,
        // maybe it's from win32u.dll
        //
        DllHandle = LoadLibraryA("win32u.dll");

        if (!DllHandle)
        {
            ShowMessages("err, failed to load win32u.dll\n");
            return NULL;
        }

        TargetFunc = GetProcAddress(DllHandle, NtFunctionName);

        if (!TargetFunc)
        {
            ShowMessages("err, failed to get address of %s\n", NtFunctionName);
            return NULL;
        }
    }

    //
    // By default, we send 30 bytes to the disassembler,
    // since usually the syscall handler is less than 30 bytes,
    // and we want to avoid disassembling another function
    //
    return HyperDbgGetImmediateValueOnEaxForSyscallNumber((UCHAR *)TargetFunc, 30, TRUE);
}
