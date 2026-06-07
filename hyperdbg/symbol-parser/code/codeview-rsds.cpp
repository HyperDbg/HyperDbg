/**
 * @file codeview-rsds.cpp
 * @author jtaw5649
 * @brief Bounded in-memory CodeView RSDS parser
 * @details
 * @version 0.19
 * @date 2026-06-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Helper function to check if the specified offset and length are within the bounds of the image size
 *
 * @param ImageSize The size of the image in bytes
 * @param Offset The offset to check
 * @param Length The length to check
 *
 * @return BOOLEAN TRUE if the offset and length are within bounds, FALSE otherwise
 */
static BOOLEAN
SymRsdsHasRange(SIZE_T ImageSize, SIZE_T Offset, SIZE_T Length)
{
    return Offset <= ImageSize && Length <= ImageSize - Offset;
}

/**
 * @brief Helper function to safely add two SIZE_T values and check for overflow
 *
 * @param Left The first value to add
 * @param Right The second value to add
 * @param Result An output pointer to receive the result of the addition if successful
 *
 * @return BOOLEAN TRUE if the addition was successful without overflow, FALSE otherwise
 */
static BOOLEAN
SymRsdsAddSize(SIZE_T Left, SIZE_T Right, SIZE_T * Result)
{
    if (Result == NULL || Right > (SIZE_T)-1 - Left)
    {
        return FALSE;
    }

    *Result = Left + Right;
    return TRUE;
}

/**
 * @brief Helper function to clear the output parameters for the PDB file name, GUID, and age
 *
 * @param PdbFileName An optional output buffer for the PDB file name. If not NULL, it will be set to an empty string
 * @param PdbFileNameSize The size of the PDB file name buffer in bytes. Must be greater than 0 if PdbFileName is not NULL
 * @param Guid An optional output pointer for the GUID. If not NULL, it will be zeroed out
 * @param Age An optional output pointer for the age. If not NULL, it will be set to 0
 *
 * @return VOID
 */
static VOID
SymRsdsClearOutputs(CHAR * PdbFileName, SIZE_T PdbFileNameSize, GUID * Guid, DWORD * Age)
{
    if (PdbFileName != NULL && PdbFileNameSize != 0)
    {
        PdbFileName[0] = '\0';
    }

    if (Guid != NULL)
    {
        ZeroMemory(Guid, sizeof(*Guid));
    }

    if (Age != NULL)
    {
        *Age = 0;
    }
}

/**
 * @brief Helper function to get a pointer to a specified offset and length within the image, while checking bounds
 *
 * @param ImageBase The base address of the image in memory
 * @param ImageSize The size of the image in bytes
 * @param Offset The offset to get a pointer to
 * @param Length The length of the range to check for validity
 * @param Pointer An output pointer to receive the pointer to the specified offset if valid
 *
 * @return BOOLEAN TRUE if the pointer was successfully obtained and is within bounds, FALSE otherwise
 */
static BOOLEAN
SymRsdsGetPointerAtOffset(const BYTE * ImageBase, SIZE_T ImageSize, SIZE_T Offset, SIZE_T Length, const BYTE ** Pointer)
{
    if (ImageBase == NULL || Pointer == NULL || !SymRsdsHasRange(ImageSize, Offset, Length))
    {
        return FALSE;
    }

    *Pointer = ImageBase + Offset;
    return TRUE;
}

/**
 * @brief Helper function to extract the SizeOfHeaders field from the optional header, handling both 32-bit and 64-bit formats
 *
 * @param OptionalHeader A pointer to the optional header data in memory
 * @param Is32Bit A boolean indicating whether the optional header is in 32-bit format (true) or 64-bit format (false)
 *
 * @return DWORD The value of the SizeOfHeaders field from the optional header
 */
static DWORD
SymRsdsGetSizeOfHeaders(const BYTE * OptionalHeader, BOOLEAN Is32Bit)
{
    if (Is32Bit)
    {
        return ((const IMAGE_OPTIONAL_HEADER32 *)OptionalHeader)->SizeOfHeaders;
    }

    return ((const IMAGE_OPTIONAL_HEADER64 *)OptionalHeader)->SizeOfHeaders;
}

/**
 * @brief Helper function to extract the IMAGE_DATA_DIRECTORY entry for the debug directory from the optional header, handling both 32-bit and 64-bit formats
 *
 * @param OptionalHeader A pointer to the optional header data in memory
 * @param Is32Bit A boolean indicating whether the optional header is in 32-bit format (true) or 64-bit format (false)
 *
 * @return IMAGE_DATA_DIRECTORY The IMAGE_DATA_DIRECTORY entry for the debug directory from the optional header
 */
static IMAGE_DATA_DIRECTORY
SymRsdsGetDebugDataDirectory(const BYTE * OptionalHeader, BOOLEAN Is32Bit)
{
    if (Is32Bit)
    {
        return ((const IMAGE_OPTIONAL_HEADER32 *)OptionalHeader)->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
    }

    return ((const IMAGE_OPTIONAL_HEADER64 *)OptionalHeader)->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
}

/**
 * @brief Helper function to extract the NumberOfRvaAndSizes field from the optional header, handling both 32-bit and 64-bit formats
 *
 * @param OptionalHeader A pointer to the optional header data in memory
 * @param Is32Bit A boolean indicating whether the optional header is in 32-bit format (true) or 64-bit format (false)
 *
 * @return DWORD The value of the NumberOfRvaAndSizes field from the optional header
 */
static DWORD
SymRsdsGetNumberOfRvaAndSizes(const BYTE * OptionalHeader, BOOLEAN Is32Bit)
{
    if (Is32Bit)
    {
        return ((const IMAGE_OPTIONAL_HEADER32 *)OptionalHeader)->NumberOfRvaAndSizes;
    }

    return ((const IMAGE_OPTIONAL_HEADER64 *)OptionalHeader)->NumberOfRvaAndSizes;
}

/**
 * @brief Helper function to convert an RVA to a file offset by checking the section headers and bounds of the image
 *
 * @param ImageBase The base address of the image in memory
 * @param ImageSize The size of the image in bytes
 * @param FileHeader A pointer to the IMAGE_FILE_HEADER structure from the PE headers
 * @param SectionHeaders A pointer to the first IMAGE_SECTION_HEADER in the section header array from the PE headers
 * @param SizeOfHeaders The value of the SizeOfHeaders field from the optional header
 * @param Rva The RVA to convert to a file offset
 * @param Length The length of the range starting at Rva to check for validity
 * @param FileOffset An output pointer to receive the calculated file offset if successful
 *
 * @return BOOLEAN TRUE if the RVA was successfully converted to a file offset and is within bounds, FALSE otherwise
 */
static BOOLEAN
SymRsdsRvaToFileOffset(const BYTE *                 ImageBase,
                       SIZE_T                       ImageSize,
                       const IMAGE_FILE_HEADER *    FileHeader,
                       const IMAGE_SECTION_HEADER * SectionHeaders,
                       DWORD                        SizeOfHeaders,
                       DWORD                        Rva,
                       DWORD                        Length,
                       SIZE_T *                     FileOffset)
{
    if (ImageBase == NULL || FileHeader == NULL || SectionHeaders == NULL || FileOffset == NULL)
    {
        return FALSE;
    }

    SIZE_T HeaderEnd = 0;

    if (SymRsdsAddSize((SIZE_T)Rva, (SIZE_T)Length, &HeaderEnd) && SizeOfHeaders <= ImageSize && Rva < SizeOfHeaders &&
        HeaderEnd <= SizeOfHeaders && SymRsdsHasRange(ImageSize, (SIZE_T)Rva, (SIZE_T)Length))
    {
        *FileOffset = (SIZE_T)Rva;
        return TRUE;
    }

    for (WORD Index = 0; Index < FileHeader->NumberOfSections; Index++)
    {
        const IMAGE_SECTION_HEADER * SectionHeader = &SectionHeaders[Index];
        DWORD                        SectionSpan   = max(SectionHeader->Misc.VirtualSize, SectionHeader->SizeOfRawData);

        if (SectionSpan == 0 || Rva < SectionHeader->VirtualAddress)
        {
            continue;
        }

        DWORD Delta = Rva - SectionHeader->VirtualAddress;
        if (Delta >= SectionSpan || Length > SectionSpan - Delta)
        {
            continue;
        }

        if (Delta > SectionHeader->SizeOfRawData || Length > SectionHeader->SizeOfRawData - Delta)
        {
            return FALSE;
        }

        SIZE_T RawOffset = 0;

        if (!SymRsdsAddSize((SIZE_T)SectionHeader->PointerToRawData, (SIZE_T)Delta, &RawOffset) ||
            !SymRsdsHasRange(ImageSize, RawOffset, (SIZE_T)Length))
        {
            return FALSE;
        }

        *FileOffset = RawOffset;
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief Helper function to convert an RVA to a loaded image offset by checking the bounds of the image
 *
 * @param ImageSize The size of the image in bytes
 * @param Rva The RVA to convert to a loaded image offset
 * @param Length The length of the range starting at Rva to check for validity
 * @param LoadedOffset An output pointer to receive the calculated loaded image offset if successful
 *
 * @return BOOLEAN TRUE if the RVA was successfully converted to a loaded image offset and is within bounds, FALSE otherwise
 */
static BOOLEAN
SymRsdsRvaToLoadedOffset(SIZE_T ImageSize, DWORD Rva, DWORD Length, SIZE_T * LoadedOffset)
{
    if (LoadedOffset == NULL || !SymRsdsHasRange(ImageSize, (SIZE_T)Rva, (SIZE_T)Length))
    {
        return FALSE;
    }

    *LoadedOffset = (SIZE_T)Rva;
    return TRUE;
}

/**
 * @brief Helper function to extract the base name from a file path and copy it to the output buffer, ensuring it fits within the buffer size
 *
 * @param Path The input file path string
 * @param PathLength The length of the input file path string in bytes
 * @param PdbFileName The output buffer to receive the base name of the file. Must be at least PdbFileNameSize bytes
 * @param PdbFileNameSize The size of the PdbFileName buffer in bytes
 *
 * @return BOOLEAN TRUE if the base name was successfully extracted and copied to the output buffer, FALSE otherwise (e.g., if inputs are invalid or base name does not fit in buffer)
 */
static BOOLEAN
SymRsdsCopyBasename(const CHAR * Path, SIZE_T PathLength, CHAR * PdbFileName, SIZE_T PdbFileNameSize)
{
    if (Path == NULL || PdbFileName == NULL || PdbFileNameSize == 0)
    {
        return FALSE;
    }

    const CHAR * BaseName   = Path;
    SIZE_T       BaseLength = PathLength;

    for (SIZE_T Index = 0; Index < PathLength; Index++)
    {
        if (Path[Index] == '\\' || Path[Index] == '/')
        {
            BaseName   = Path + Index + 1;
            BaseLength = PathLength - Index - 1;
        }
    }

    if (BaseLength == 0 || BaseLength >= PdbFileNameSize)
    {
        return FALSE;
    }

    CopyMemory(PdbFileName, BaseName, BaseLength);
    PdbFileName[BaseLength] = '\0';
    return TRUE;
}

/**
 * @brief Helper function to parse the RSDS CodeView debug information from the provided data and extract the PDB file name, GUID, and age
 *
 * @param CodeViewData A pointer to the CodeView debug information data in memory
 * @param CodeViewSize The size of the CodeView debug information data in bytes
 * @param PdbFileName An output buffer to receive the base name of the PDB file extracted from the CodeView data. Must be at least PdbFileNameSize bytes
 * @param PdbFileNameSize The size of the PdbFileName buffer in bytes
 * @param Guid An output pointer to receive the GUID extracted from the CodeView data
 * @param Age An output pointer to receive the age extracted from the CodeView data
 *
 * @return BOOLEAN TRUE if the CodeView data was successfully parsed and information was extracted, FALSE otherwise (e.g., if data is invalid or does not contain valid RSDS information)
 */
static BOOLEAN
SymRsdsTryParseCodeView(const BYTE * CodeViewData,
                        DWORD        CodeViewSize,
                        CHAR *       PdbFileName,
                        SIZE_T       PdbFileNameSize,
                        GUID *       Guid,
                        DWORD *      Age)
{
    static constexpr DWORD RsdsSignature  = 0x53445352;
    static constexpr DWORD RsdsHeaderSize = sizeof(DWORD) + sizeof(GUID) + sizeof(DWORD);

    if (CodeViewData == NULL || CodeViewSize < RsdsHeaderSize || *(const DWORD *)CodeViewData != RsdsSignature)
    {
        return FALSE;
    }

    const CHAR * Path       = (const CHAR *)(CodeViewData + RsdsHeaderSize);
    SIZE_T       PathLimit  = (SIZE_T)CodeViewSize - RsdsHeaderSize;
    SIZE_T       PathLength = 0;

    while (PathLength < PathLimit && Path[PathLength] != '\0')
    {
        PathLength++;
    }

    if (PathLength == PathLimit)
    {
        return FALSE;
    }

    if (!SymRsdsCopyBasename(Path, PathLength, PdbFileName, PdbFileNameSize))
    {
        return FALSE;
    }

    CopyMemory(Guid, CodeViewData + sizeof(DWORD), sizeof(*Guid));
    CopyMemory(Age, CodeViewData + sizeof(DWORD) + sizeof(GUID), sizeof(*Age));
    return TRUE;
}

/**
 * @brief Internal helper function to extract CodeView RSDS information from a PE image in memory, with options for loaded layout parsing
 *
 * @param ImageBase The base address of the PE image in memory
 * @param ImageSize The size of the PE image in bytes
 * @param PdbFileName An output buffer to receive the base name of the PDB file extracted from the CodeView data. Must be at least PdbFileNameSize bytes
 * @param PdbFileNameSize The size of the PdbFileName buffer in bytes
 * @param Guid An output pointer to receive the GUID extracted from the CodeView data
 * @param Age An output pointer to receive the age extracted from the CodeView data
 * @param LoadedLayout A boolean indicating whether to parse the image as if it is loaded in memory (true) or as a file on disk (false), which affects how RVAs are interpreted
 *
 * @return BOOLEAN TRUE if the CodeView RSDS information was successfully extracted, FALSE otherwise (e.g., if image is invalid or does not contain valid RSDS information)
 */
static BOOLEAN
SymExtractCodeViewRsdsInfoFromPeImageInternal(const BYTE * ImageBase,
                                              SIZE_T       ImageSize,
                                              CHAR *       PdbFileName,
                                              SIZE_T       PdbFileNameSize,
                                              GUID *       Guid,
                                              DWORD *      Age,
                                              BOOLEAN      LoadedLayout)
{
    SymRsdsClearOutputs(PdbFileName, PdbFileNameSize, Guid, Age);

    if (ImageBase == NULL || PdbFileName == NULL || PdbFileNameSize == 0 || Guid == NULL || Age == NULL)
    {
        return FALSE;
    }

    if (!SymRsdsHasRange(ImageSize, 0, sizeof(IMAGE_DOS_HEADER)))
    {
        return FALSE;
    }

    const IMAGE_DOS_HEADER * DosHeader = (const IMAGE_DOS_HEADER *)ImageBase;
    if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE || DosHeader->e_lfanew < 0)
    {
        return FALSE;
    }

    SIZE_T NtHeaderOffset = (SIZE_T)DosHeader->e_lfanew;
    if (!SymRsdsHasRange(ImageSize, NtHeaderOffset, sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER)))
    {
        return FALSE;
    }

    const BYTE * NtHeaders = ImageBase + NtHeaderOffset;
    if (*(const DWORD *)NtHeaders != IMAGE_NT_SIGNATURE)
    {
        return FALSE;
    }

    const IMAGE_FILE_HEADER * FileHeader           = (const IMAGE_FILE_HEADER *)(NtHeaders + sizeof(DWORD));
    SIZE_T                    OptionalHeaderOffset = NtHeaderOffset + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER);

    if (FileHeader->SizeOfOptionalHeader < sizeof(WORD) ||
        !SymRsdsHasRange(ImageSize, OptionalHeaderOffset, FileHeader->SizeOfOptionalHeader))
    {
        return FALSE;
    }

    const BYTE * OptionalHeader            = ImageBase + OptionalHeaderOffset;
    WORD         OptionalHeaderMagic       = *(const WORD *)OptionalHeader;
    BOOLEAN      Is32Bit                   = FALSE;
    SIZE_T       MinimumOptionalHeaderSize = 0;

    if (OptionalHeaderMagic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        Is32Bit                   = TRUE;
        MinimumOptionalHeaderSize = sizeof(IMAGE_OPTIONAL_HEADER32);
    }
    else if (OptionalHeaderMagic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        MinimumOptionalHeaderSize = sizeof(IMAGE_OPTIONAL_HEADER64);
    }
    else
    {
        return FALSE;
    }

    if (FileHeader->SizeOfOptionalHeader < MinimumOptionalHeaderSize)
    {
        return FALSE;
    }

    SIZE_T SectionTableOffset = OptionalHeaderOffset + FileHeader->SizeOfOptionalHeader;
    SIZE_T SectionTableSize   = (SIZE_T)FileHeader->NumberOfSections * sizeof(IMAGE_SECTION_HEADER);

    if (!LoadedLayout && FileHeader->NumberOfSections != 0 && SectionTableSize / sizeof(IMAGE_SECTION_HEADER) != FileHeader->NumberOfSections)
    {
        return FALSE;
    }

    if (!LoadedLayout && !SymRsdsHasRange(ImageSize, SectionTableOffset, SectionTableSize))
    {
        return FALSE;
    }

    if (SymRsdsGetNumberOfRvaAndSizes(OptionalHeader, Is32Bit) <= IMAGE_DIRECTORY_ENTRY_DEBUG)
    {
        return FALSE;
    }

    IMAGE_DATA_DIRECTORY DebugDirectory = SymRsdsGetDebugDataDirectory(OptionalHeader, Is32Bit);
    if (DebugDirectory.VirtualAddress == 0 || DebugDirectory.Size < sizeof(IMAGE_DEBUG_DIRECTORY) ||
        DebugDirectory.Size % sizeof(IMAGE_DEBUG_DIRECTORY) != 0)
    {
        return FALSE;
    }

    const IMAGE_SECTION_HEADER * SectionHeaders = LoadedLayout ? NULL : (const IMAGE_SECTION_HEADER *)(ImageBase + SectionTableOffset);
    SIZE_T                       DebugOffset    = 0;

    if (LoadedLayout)
    {
        if (!SymRsdsRvaToLoadedOffset(ImageSize, DebugDirectory.VirtualAddress, DebugDirectory.Size, &DebugOffset))
        {
            return FALSE;
        }
    }
    else if (!SymRsdsRvaToFileOffset(ImageBase,
                                     ImageSize,
                                     FileHeader,
                                     SectionHeaders,
                                     SymRsdsGetSizeOfHeaders(OptionalHeader, Is32Bit),
                                     DebugDirectory.VirtualAddress,
                                     DebugDirectory.Size,
                                     &DebugOffset))
    {
        return FALSE;
    }

    const BYTE * DebugBytes = NULL;
    if (!SymRsdsGetPointerAtOffset(ImageBase, ImageSize, DebugOffset, DebugDirectory.Size, &DebugBytes))
    {
        return FALSE;
    }

    DWORD DebugEntryCount = DebugDirectory.Size / sizeof(IMAGE_DEBUG_DIRECTORY);
    for (DWORD Index = 0; Index < DebugEntryCount; Index++)
    {
        const IMAGE_DEBUG_DIRECTORY * DebugEntry = ((const IMAGE_DEBUG_DIRECTORY *)DebugBytes) + Index;
        if (DebugEntry->Type != IMAGE_DEBUG_TYPE_CODEVIEW || DebugEntry->SizeOfData < sizeof(DWORD) + sizeof(GUID) + sizeof(DWORD))
        {
            continue;
        }

        SIZE_T       CodeViewOffset = 0;
        const BYTE * CodeViewData   = NULL;

        BOOLEAN IsCodeViewMapped = LoadedLayout ? SymRsdsRvaToLoadedOffset(ImageSize, DebugEntry->AddressOfRawData, DebugEntry->SizeOfData, &CodeViewOffset) : SymRsdsRvaToFileOffset(ImageBase, ImageSize, FileHeader, SectionHeaders, SymRsdsGetSizeOfHeaders(OptionalHeader, Is32Bit), DebugEntry->AddressOfRawData, DebugEntry->SizeOfData, &CodeViewOffset);

        if (!IsCodeViewMapped || !SymRsdsGetPointerAtOffset(ImageBase, ImageSize, CodeViewOffset, DebugEntry->SizeOfData, &CodeViewData))
        {
            continue;
        }

        if (SymRsdsTryParseCodeView(CodeViewData, DebugEntry->SizeOfData, PdbFileName, PdbFileNameSize, Guid, Age))
        {
            return TRUE;
        }

        SymRsdsClearOutputs(PdbFileName, PdbFileNameSize, Guid, Age);
    }

    return FALSE;
}

/**
 * @brief Extracts CodeView RSDS information from a PE image in memory, interpreting the image as it would be laid out on disk
 *
 * @param ImageBase The base address of the PE image in memory
 * @param ImageSize The size of the PE image in bytes
 * @param PdbFileName An output buffer to receive the base name of the PDB file extracted from the CodeView data. Must be at least PdbFileNameSize bytes
 * @param PdbFileNameSize The size of the PdbFileName buffer in bytes
 * @param Guid An output pointer to receive the GUID extracted from the CodeView data
 * @param Age An output pointer to receive the age extracted from the CodeView data
 *
 * @return BOOLEAN TRUE if the CodeView RSDS information was successfully extracted, FALSE otherwise (e.g., if image is invalid or does not contain valid RSDS information)
 */
BOOLEAN
SymExtractCodeViewRsdsInfoFromPeImage(const BYTE * ImageBase,
                                      SIZE_T       ImageSize,
                                      CHAR *       PdbFileName,
                                      SIZE_T       PdbFileNameSize,
                                      GUID *       Guid,
                                      DWORD *      Age)
{
    return SymExtractCodeViewRsdsInfoFromPeImageInternal(ImageBase, ImageSize, PdbFileName, PdbFileNameSize, Guid, Age, FALSE);
}

/**
 * @brief Extracts CodeView RSDS information from a PE image in memory, interpreting the image as it would be laid out in memory when loaded (i.e., using loaded image offsets)
 *
 * @param ImageBase The base address of the PE image in memory
 * @param ImageSize The size of the PE image in bytes
 * @param PdbFileName An output buffer to receive the base name of the PDB file extracted from the CodeView data. Must be at least PdbFileNameSize bytes
 * @param PdbFileNameSize The size of the PdbFileName buffer in bytes
 * @param Guid An output pointer to receive the GUID extracted from the CodeView data
 * @param Age An output pointer to receive the age extracted from the CodeView data
 *
 * @return BOOLEAN TRUE if the CodeView RSDS information was successfully extracted, FALSE otherwise (e.g., if image is invalid or does not contain valid RSDS information)
 */
BOOLEAN
SymExtractCodeViewRsdsInfoFromLoadedPeImage(const BYTE * ImageBase,
                                            SIZE_T       ImageSize,
                                            CHAR *       PdbFileName,
                                            SIZE_T       PdbFileNameSize,
                                            GUID *       Guid,
                                            DWORD *      Age)
{
    return SymExtractCodeViewRsdsInfoFromPeImageInternal(ImageBase, ImageSize, PdbFileName, PdbFileNameSize, Guid, Age, TRUE);
}
