/**
 * @file pe-image-reader.cpp
 * @author jtaw5649
 * @brief Bounded in-memory Portable Executable reader
 * @details
 * @version 0.19
 * @date 2026-06-01
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Checks whether the byte range [Offset, Offset + Length) lies entirely within an image buffer
 *
 * Validates that neither the offset nor the combined offset-plus-length overflows
 * and that both fall within the bounds of the image.
 *
 * @param ImageSize Total size of the image buffer in bytes
 * @param Offset Starting byte offset to test
 * @param Length Number of bytes in the range
 *
 * @return BOOLEAN TRUE if the range is valid, FALSE if it exceeds the image bounds
 */
static BOOLEAN
PeImageReaderHasRange(SIZE_T ImageSize, SIZE_T Offset, SIZE_T Length)
{
    return Offset <= ImageSize && Length <= ImageSize - Offset;
}

/**
 * @brief Adds two SIZE_T values with overflow detection
 *
 * Returns FALSE without modifying Result when the addition would overflow;
 * otherwise writes the sum to *Result and returns TRUE.
 *
 * @param Left First operand
 * @param Right Second operand
 * @param Result Output pointer that receives the sum on success; must not be NULL
 *
 * @return BOOLEAN TRUE if the addition succeeded, FALSE on overflow or NULL pointer
 */
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

/**
 * @brief Retrieves the SizeOfHeaders value from the PE optional header
 *
 * Reads the field from IMAGE_OPTIONAL_HEADER32 or IMAGE_OPTIONAL_HEADER64
 * depending on the bitness recorded in the reader.
 *
 * @param Reader Pointer to an initialized PE_IMAGE_READER; must not be NULL
 *
 * @return DWORD The SizeOfHeaders value from the optional header
 */
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

/**
 * @brief Parses and validates all PE headers in an in-memory image buffer
 *
 * Verifies the DOS signature, the NT signature, the optional header magic,
 * and ensures all headers and the section table fit within the supplied buffer.
 * On success the Reader structure is populated with pointers into ImageBase.
 *
 * @param ImageBase Pointer to the start of the image buffer; must not be NULL
 * @param ImageSize Size of the buffer in bytes
 * @param Reader Output structure to populate on success; must not be NULL
 *
 * @return BOOLEAN TRUE if the image was parsed successfully, FALSE on any
 *         validation failure or NULL argument
 */
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

/**
 * @brief Returns whether the PE image is a 32-bit (PE32) image
 *
 * Examines the Is32Bit flag populated by PeImageReaderInitialize.
 * A return value of FALSE means either the reader is NULL or the image is PE32+.
 *
 * @param Reader Pointer to an initialized PE_IMAGE_READER
 *
 * @return BOOLEAN TRUE for PE32 (32-bit), FALSE for PE32+ (64-bit) or NULL reader
 */
BOOLEAN
PeImageReaderIs32Bit(PPE_IMAGE_READER Reader)
{
    if (Reader == NULL)
    {
        return FALSE;
    }

    return Reader->Is32Bit;
}

/**
 * @brief Returns a validated pointer into the image at a raw file offset
 *
 * Verifies that the range [Offset, Offset + Length) lies within the image
 * buffer before setting *Pointer. Use this function when working with raw
 * file offsets rather than virtual addresses.
 *
 * @param Reader  Pointer to an initialized PE_IMAGE_READER; must not be NULL
 * @param Offset  Raw file offset from the start of the image
 * @param Length  Number of bytes that must be accessible at the offset
 * @param Pointer Output pointer set to ImageBase + Offset on success; must not be NULL
 *
 * @return BOOLEAN TRUE on success, FALSE on invalid arguments or out-of-bounds offset
 */
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

/**
 * @brief Copies the section name from a section header into a null-terminated buffer
 *
 * The PE section name field (IMAGE_SIZEOF_SHORT_NAME bytes) is not required to be
 * null-terminated when it uses all 8 bytes. This function always appends a null
 * terminator and truncates to NameBufferSize - 1 characters if necessary.
 *
 * @param SectionHeader  Pointer to the section header to read; must not be NULL
 * @param NameBuffer     Destination buffer for the null-terminated name; must not be NULL
 * @param NameBufferSize Size of NameBuffer in bytes; must be at least 1
 *
 * @return BOOLEAN TRUE on success, FALSE on NULL arguments or zero-length buffer
 */
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

/**
 * @brief Translates a relative virtual address (RVA) to a raw file offset
 *
 * First checks whether the RVA falls within the PE headers (before any section),
 * in which case the file offset equals the RVA. Otherwise iterates the section
 * table to find the section that contains the range [Rva, Rva + Length) and
 * computes the corresponding raw offset via PointerToRawData. Returns FALSE if
 * no section contains the range, if the raw data mapping is out of bounds, or if
 * any arithmetic overflows.
 *
 * @param Reader     Pointer to an initialized PE_IMAGE_READER; must not be NULL
 * @param Rva        Relative virtual address to translate
 * @param Length     Number of bytes that must be accessible at the translated offset
 * @param FileOffset Output pointer that receives the raw file offset on success; must not be NULL
 *
 * @return BOOLEAN TRUE if the RVA was translated successfully, FALSE otherwise
 */
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
