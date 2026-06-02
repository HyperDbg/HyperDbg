/**
 * @file pe-image-reader.h
 * @author jtaw5649
 * @brief Bounded in-memory Portable Executable reader
 * @details
 * @version 0.1
 * @date 2026-06-01
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//                  Structures                  //
//////////////////////////////////////////////////

typedef struct _PE_IMAGE_READER
{
    const BYTE *                 ImageBase;
    SIZE_T                       ImageSize;
    const IMAGE_DOS_HEADER *     DosHeader;
    const BYTE *                 NtHeaders;
    const IMAGE_FILE_HEADER *    FileHeader;
    const IMAGE_SECTION_HEADER * SectionHeaders;
    WORD                         OptionalHeaderMagic;
    BOOLEAN                      Is32Bit;
} PE_IMAGE_READER, *PPE_IMAGE_READER;

//////////////////////////////////////////////////
//                  Functions                   //
//////////////////////////////////////////////////

BOOLEAN
PeImageReaderInitialize(const BYTE * ImageBase, SIZE_T ImageSize, PPE_IMAGE_READER Reader);

BOOLEAN
PeImageReaderIs32Bit(PPE_IMAGE_READER Reader);

BOOLEAN
PeImageReaderGetPointerAtOffset(PPE_IMAGE_READER Reader, SIZE_T Offset, SIZE_T Length, const BYTE ** Pointer);

BOOLEAN
PeImageReaderGetSectionName(const IMAGE_SECTION_HEADER * SectionHeader, CHAR * NameBuffer, SIZE_T NameBufferSize);

BOOLEAN
PeImageReaderRvaToFileOffset(PPE_IMAGE_READER Reader, DWORD Rva, DWORD Length, PSIZE_T FileOffset);
