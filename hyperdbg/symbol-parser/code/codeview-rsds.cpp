/**
 * @file codeview-rsds.cpp
 * @author jtaw5649
 * @brief Bounded in-memory CodeView RSDS parser
 * @details
 * @version 0.1
 * @date 2026-06-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#include "header/codeview-rsds.h"

static BOOLEAN
SymRsdsHasRange(SIZE_T ImageSize, SIZE_T Offset, SIZE_T Length)
{
    return Offset <= ImageSize && Length <= ImageSize - Offset;
}

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

static DWORD
SymRsdsGetSizeOfHeaders(const BYTE * OptionalHeader, BOOLEAN Is32Bit)
{
    if (Is32Bit)
    {
        return ((const IMAGE_OPTIONAL_HEADER32 *)OptionalHeader)->SizeOfHeaders;
    }

    return ((const IMAGE_OPTIONAL_HEADER64 *)OptionalHeader)->SizeOfHeaders;
}

static IMAGE_DATA_DIRECTORY
SymRsdsGetDebugDataDirectory(const BYTE * OptionalHeader, BOOLEAN Is32Bit)
{
    if (Is32Bit)
    {
        return ((const IMAGE_OPTIONAL_HEADER32 *)OptionalHeader)->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
    }

    return ((const IMAGE_OPTIONAL_HEADER64 *)OptionalHeader)->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
}

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

BOOLEAN
SymExtractCodeViewRsdsInfoFromPeImage(const BYTE * ImageBase,
                                      SIZE_T       ImageSize,
                                      CHAR *       PdbFileName,
                                      SIZE_T       PdbFileNameSize,
                                      GUID *       Guid,
                                      DWORD *      Age)
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

    if (FileHeader->NumberOfSections != 0 && SectionTableSize / sizeof(IMAGE_SECTION_HEADER) != FileHeader->NumberOfSections)
    {
        return FALSE;
    }

    if (!SymRsdsHasRange(ImageSize, SectionTableOffset, SectionTableSize))
    {
        return FALSE;
    }

    IMAGE_DATA_DIRECTORY DebugDirectory = SymRsdsGetDebugDataDirectory(OptionalHeader, Is32Bit);
    if (DebugDirectory.VirtualAddress == 0 || DebugDirectory.Size < sizeof(IMAGE_DEBUG_DIRECTORY) ||
        DebugDirectory.Size % sizeof(IMAGE_DEBUG_DIRECTORY) != 0)
    {
        return FALSE;
    }

    const IMAGE_SECTION_HEADER * SectionHeaders = (const IMAGE_SECTION_HEADER *)(ImageBase + SectionTableOffset);
    SIZE_T                       DebugOffset    = 0;

    if (!SymRsdsRvaToFileOffset(ImageBase,
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

        if (!SymRsdsRvaToFileOffset(ImageBase,
                                    ImageSize,
                                    FileHeader,
                                    SectionHeaders,
                                    SymRsdsGetSizeOfHeaders(OptionalHeader, Is32Bit),
                                    DebugEntry->AddressOfRawData,
                                    DebugEntry->SizeOfData,
                                    &CodeViewOffset) ||
            !SymRsdsGetPointerAtOffset(ImageBase, ImageSize, CodeViewOffset, DebugEntry->SizeOfData, &CodeViewData))
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
