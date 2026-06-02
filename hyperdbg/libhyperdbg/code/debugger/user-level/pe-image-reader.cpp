/**
 * @file pe-image-reader.cpp
 * @author jtaw5649
 * @brief Bounded in-memory Portable Executable reader
 * @details
 * @version 0.1
 * @date 2026-06-01
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#include "header/pe-image-reader.h"

static BOOLEAN
PeImageReaderHasRange(SIZE_T ImageSize, SIZE_T Offset, SIZE_T Length)
{
    return Offset <= ImageSize && Length <= ImageSize - Offset;
}

static BOOLEAN
PeImageReaderAddSize(SIZE_T Left, SIZE_T Right, SIZE_T * Result)
{
    if (Result == NULL || Right > (SIZE_T)-1 - Left)
    {
        return FALSE;
    }

    *Result = Left + Right;
    return TRUE;
}

static DWORD
PeImageReaderGetSizeOfHeaders(PPE_IMAGE_READER Reader)
{
    const BYTE * OptionalHeader = Reader->NtHeaders + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER);

    if (Reader->Is32Bit)
    {
        return ((const IMAGE_OPTIONAL_HEADER32 *)OptionalHeader)->SizeOfHeaders;
    }

    return ((const IMAGE_OPTIONAL_HEADER64 *)OptionalHeader)->SizeOfHeaders;
}

BOOLEAN
PeImageReaderInitialize(const BYTE * ImageBase, SIZE_T ImageSize, PPE_IMAGE_READER Reader)
{
    if (ImageBase == NULL || Reader == NULL)
    {
        return FALSE;
    }

    ZeroMemory(Reader, sizeof(*Reader));

    if (!PeImageReaderHasRange(ImageSize, 0, sizeof(IMAGE_DOS_HEADER)))
    {
        return FALSE;
    }

    const IMAGE_DOS_HEADER * DosHeader = (const IMAGE_DOS_HEADER *)ImageBase;
    if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE || DosHeader->e_lfanew < 0)
    {
        return FALSE;
    }

    SIZE_T NtHeaderOffset = (SIZE_T)DosHeader->e_lfanew;
    if (!PeImageReaderHasRange(ImageSize, NtHeaderOffset, sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER)))
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
        !PeImageReaderHasRange(ImageSize, OptionalHeaderOffset, FileHeader->SizeOfOptionalHeader))
    {
        return FALSE;
    }

    WORD    OptionalHeaderMagic       = *(const WORD *)(ImageBase + OptionalHeaderOffset);
    BOOLEAN Is32Bit                   = FALSE;
    SIZE_T  MinimumOptionalHeaderSize = 0;

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

    if (!PeImageReaderHasRange(ImageSize, SectionTableOffset, SectionTableSize))
    {
        return FALSE;
    }

    Reader->ImageBase           = ImageBase;
    Reader->ImageSize           = ImageSize;
    Reader->DosHeader           = DosHeader;
    Reader->NtHeaders           = NtHeaders;
    Reader->FileHeader          = FileHeader;
    Reader->SectionHeaders      = (const IMAGE_SECTION_HEADER *)(ImageBase + SectionTableOffset);
    Reader->OptionalHeaderMagic = OptionalHeaderMagic;
    Reader->Is32Bit             = Is32Bit;

    return TRUE;
}

BOOLEAN
PeImageReaderIs32Bit(PPE_IMAGE_READER Reader)
{
    if (Reader == NULL)
    {
        return FALSE;
    }

    return Reader->Is32Bit;
}

BOOLEAN
PeImageReaderGetPointerAtOffset(PPE_IMAGE_READER Reader, SIZE_T Offset, SIZE_T Length, const BYTE ** Pointer)
{
    if (Reader == NULL || Reader->ImageBase == NULL || Pointer == NULL || !PeImageReaderHasRange(Reader->ImageSize, Offset, Length))
    {
        return FALSE;
    }

    *Pointer = Reader->ImageBase + Offset;
    return TRUE;
}

BOOLEAN
PeImageReaderGetSectionName(const IMAGE_SECTION_HEADER * SectionHeader, CHAR * NameBuffer, SIZE_T NameBufferSize)
{
    if (SectionHeader == NULL || NameBuffer == NULL || NameBufferSize == 0)
    {
        return FALSE;
    }

    SIZE_T NameLength = 0;
    while (NameLength < IMAGE_SIZEOF_SHORT_NAME && SectionHeader->Name[NameLength] != '\0')
    {
        NameLength++;
    }

    SIZE_T CopyLength = NameLength;
    if (CopyLength >= NameBufferSize)
    {
        CopyLength = NameBufferSize - 1;
    }

    CopyMemory(NameBuffer, SectionHeader->Name, CopyLength);
    NameBuffer[CopyLength] = '\0';

    return TRUE;
}

BOOLEAN
PeImageReaderRvaToFileOffset(PPE_IMAGE_READER Reader, DWORD Rva, DWORD Length, PSIZE_T FileOffset)
{
    if (Reader == NULL || Reader->ImageBase == NULL || Reader->FileHeader == NULL || Reader->SectionHeaders == NULL || FileOffset == NULL)
    {
        return FALSE;
    }

    DWORD  SizeOfHeaders = PeImageReaderGetSizeOfHeaders(Reader);
    SIZE_T HeaderEnd     = 0;

    if (PeImageReaderAddSize((SIZE_T)Rva, (SIZE_T)Length, &HeaderEnd) &&
        SizeOfHeaders <= Reader->ImageSize && Rva < SizeOfHeaders && HeaderEnd <= SizeOfHeaders &&
        PeImageReaderHasRange(Reader->ImageSize, (SIZE_T)Rva, (SIZE_T)Length))
    {
        *FileOffset = (SIZE_T)Rva;
        return TRUE;
    }

    for (WORD Index = 0; Index < Reader->FileHeader->NumberOfSections; Index++)
    {
        const IMAGE_SECTION_HEADER * SectionHeader = &Reader->SectionHeaders[Index];
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

        if (!PeImageReaderAddSize((SIZE_T)SectionHeader->PointerToRawData, (SIZE_T)Delta, &RawOffset) ||
            !PeImageReaderHasRange(Reader->ImageSize, RawOffset, (SIZE_T)Length))
        {
            return FALSE;
        }

        *FileOffset = RawOffset;
        return TRUE;
    }

    return FALSE;
}
