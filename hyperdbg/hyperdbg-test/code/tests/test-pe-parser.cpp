/**
 * @file test-pe-parser.cpp
 * @author jtaw5649
 * @brief Test cases for PE parser helpers
 * @details
 * @version 0.19
 * @date 2026-06-01
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

static constexpr SIZE_T PeFixtureSize  = 0x200;
static constexpr LONG   PeHeaderOffset = 0x80;

/**
 * @brief Returns the byte offset of the optional header within the test fixture buffer
 *
 * The optional header immediately follows the NT signature DWORD and the
 * IMAGE_FILE_HEADER at a fixed offset determined by PeHeaderOffset.
 *
 * @return SIZE_T Byte offset from the start of the fixture buffer
 */
static SIZE_T
PeOptionalHeaderOffset()
{
    return PeHeaderOffset + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER);
}

/**
 * @brief Returns the byte offset of the section header table within the test fixture buffer
 *
 * The section table begins immediately after the optional header, whose size
 * is supplied by the caller.
 *
 * @param OptionalHeaderSize Size in bytes of the optional header (32-bit or 64-bit variant)
 *
 * @return SIZE_T Byte offset from the start of the fixture buffer
 */
static SIZE_T
PeSectionHeaderOffset(SIZE_T OptionalHeaderSize)
{
    return PeOptionalHeaderOffset() + OptionalHeaderSize;
}

/**
 * @brief Writes a little-endian 16-bit value into a byte buffer at the given offset
 *
 * @param Buffer Pointer to the destination byte buffer
 * @param Offset Byte offset within Buffer at which to write
 * @param Value  16-bit value to write in little-endian order
 */
static VOID
WriteWord(BYTE * Buffer, SIZE_T Offset, WORD Value)
{
    Buffer[Offset]     = (BYTE)(Value & 0xff);
    Buffer[Offset + 1] = (BYTE)((Value >> 8) & 0xff);
}

/**
 * @brief Writes a little-endian 32-bit value into a byte buffer at the given offset
 *
 * @param Buffer Pointer to the destination byte buffer
 * @param Offset Byte offset within Buffer at which to write
 * @param Value  32-bit value to write in little-endian order
 */
static VOID
WriteDword(BYTE * Buffer, SIZE_T Offset, DWORD Value)
{
    Buffer[Offset]     = (BYTE)(Value & 0xff);
    Buffer[Offset + 1] = (BYTE)((Value >> 8) & 0xff);
    Buffer[Offset + 2] = (BYTE)((Value >> 16) & 0xff);
    Buffer[Offset + 3] = (BYTE)((Value >> 24) & 0xff);
}

/**
 * @brief Builds a minimal valid 64-bit PE (PE32+) fixture in the supplied buffer
 *
 * Zeroes the buffer, then writes a valid IMAGE_DOS_HEADER pointing to PeHeaderOffset,
 * an NT signature, an IMAGE_FILE_HEADER with machine type AMD64 and one section,
 * and an IMAGE_OPTIONAL_HEADER64 magic value. The result is the smallest byte
 * sequence accepted by PeImageReaderInitialize as a 64-bit PE image.
 *
 * @param Buffer Pointer to a buffer of at least PeFixtureSize bytes
 */
static VOID
BuildMinimalPe64(BYTE * Buffer)
{
    ZeroMemory(Buffer, PeFixtureSize);

    WriteWord(Buffer, 0, IMAGE_DOS_SIGNATURE);
    WriteDword(Buffer, offsetof(IMAGE_DOS_HEADER, e_lfanew), PeHeaderOffset);

    WriteDword(Buffer, PeHeaderOffset, IMAGE_NT_SIGNATURE);
    WriteWord(Buffer, PeHeaderOffset + sizeof(DWORD) + offsetof(IMAGE_FILE_HEADER, Machine), IMAGE_FILE_MACHINE_AMD64);
    WriteWord(Buffer, PeHeaderOffset + sizeof(DWORD) + offsetof(IMAGE_FILE_HEADER, NumberOfSections), 1);
    WriteWord(Buffer,
              PeHeaderOffset + sizeof(DWORD) + offsetof(IMAGE_FILE_HEADER, SizeOfOptionalHeader),
              sizeof(IMAGE_OPTIONAL_HEADER64));
    WriteWord(Buffer,
              PeHeaderOffset + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER),
              IMAGE_NT_OPTIONAL_HDR64_MAGIC);
}

/**
 * @brief Builds a minimal valid 32-bit PE (PE32) fixture in the supplied buffer
 *
 * Zeroes the buffer, then writes a valid IMAGE_DOS_HEADER pointing to PeHeaderOffset,
 * an NT signature, an IMAGE_FILE_HEADER with machine type I386 and one section,
 * and an IMAGE_OPTIONAL_HEADER32 magic value. The result is the smallest byte
 * sequence accepted by PeImageReaderInitialize as a 32-bit PE image.
 *
 * @param Buffer Pointer to a buffer of at least PeFixtureSize bytes
 */
static VOID
BuildMinimalPe32(BYTE * Buffer)
{
    ZeroMemory(Buffer, PeFixtureSize);

    WriteWord(Buffer, 0, IMAGE_DOS_SIGNATURE);
    WriteDword(Buffer, offsetof(IMAGE_DOS_HEADER, e_lfanew), PeHeaderOffset);

    WriteDword(Buffer, PeHeaderOffset, IMAGE_NT_SIGNATURE);
    WriteWord(Buffer, PeHeaderOffset + sizeof(DWORD) + offsetof(IMAGE_FILE_HEADER, Machine), IMAGE_FILE_MACHINE_I386);
    WriteWord(Buffer, PeHeaderOffset + sizeof(DWORD) + offsetof(IMAGE_FILE_HEADER, NumberOfSections), 1);
    WriteWord(Buffer,
              PeHeaderOffset + sizeof(DWORD) + offsetof(IMAGE_FILE_HEADER, SizeOfOptionalHeader),
              sizeof(IMAGE_OPTIONAL_HEADER32));
    WriteWord(Buffer,
              PeHeaderOffset + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER),
              IMAGE_NT_OPTIONAL_HDR32_MAGIC);
}

/**
 * @brief Sets the SizeOfHeaders field in the PE64 optional header of a fixture buffer
 *
 * Computes the field offset within IMAGE_OPTIONAL_HEADER64 and writes a 32-bit
 * little-endian value at that position.
 *
 * @param Buffer        Pointer to a fixture buffer previously initialised by BuildMinimalPe64
 * @param SizeOfHeaders Value to write into the SizeOfHeaders field
 */
static VOID
SetPe64OptionalHeaderSizeOfHeaders(BYTE * Buffer, DWORD SizeOfHeaders)
{
    SIZE_T Offset = PeOptionalHeaderOffset() + offsetof(IMAGE_OPTIONAL_HEADER64, SizeOfHeaders);

    WriteDword(Buffer, Offset, SizeOfHeaders);
}

/**
 * @brief Returns a pointer to the first section header in a fixture buffer
 *
 * Computes the section header table offset using OptionalHeaderSize and casts
 * the corresponding location in Buffer to IMAGE_SECTION_HEADER *.
 *
 * @param Buffer             Pointer to a fixture buffer
 * @param OptionalHeaderSize Size in bytes of the optional header used by the fixture
 *
 * @return IMAGE_SECTION_HEADER* Pointer to the first section header within the buffer
 */
static IMAGE_SECTION_HEADER *
GetFixtureSectionHeader(BYTE * Buffer, SIZE_T OptionalHeaderSize)
{
    return (IMAGE_SECTION_HEADER *)(Buffer + PeSectionHeaderOffset(OptionalHeaderSize));
}

/**
 * @brief Configures the .text section header in a fixture buffer
 *
 * Zeroes the first section header slot, writes the name ".text", and sets
 * the virtual address, virtual size, raw data pointer, and raw data size
 * fields to the supplied values.
 *
 * @param Buffer             Pointer to a fixture buffer
 * @param OptionalHeaderSize Size in bytes of the optional header used by the fixture
 * @param VirtualAddress     RVA at which the section is loaded
 * @param VirtualSize        Virtual size of the section
 * @param PointerToRawData   Raw file offset of the section data
 * @param SizeOfRawData      Size of the raw data on disk
 */
static VOID
ConfigureTextSection(BYTE * Buffer, SIZE_T OptionalHeaderSize, DWORD VirtualAddress, DWORD VirtualSize, DWORD PointerToRawData, DWORD SizeOfRawData)
{
    IMAGE_SECTION_HEADER * SectionHeader = GetFixtureSectionHeader(Buffer, OptionalHeaderSize);

    ZeroMemory(SectionHeader, sizeof(*SectionHeader));
    CopyMemory(SectionHeader->Name, ".text", sizeof(".text") - 1);
    SectionHeader->Misc.VirtualSize = VirtualSize;
    SectionHeader->VirtualAddress   = VirtualAddress;
    SectionHeader->SizeOfRawData    = SizeOfRawData;
    SectionHeader->PointerToRawData = PointerToRawData;
}

/**
 * @brief Runs all PE parser unit tests and reports pass/fail results
 *
 * Each numbered test case exercises a distinct behaviour of the PE image reader:
 *   1. A valid PE32+ image initialises successfully and reports 64-bit.
 *   2. A valid PE32 image initialises successfully and reports 32-bit.
 *   3. A corrupt DOS magic causes initialisation to fail.
 *   4. An optional header that is one byte too small causes initialisation to fail.
 *   5. An e_lfanew value that points past the buffer causes initialisation to fail.
 *   6. A valid section RVA maps to the correct raw file offset.
 *   7. Header-range RVA resolution is enforced at SizeOfHeaders boundaries.
 *   8. An 8-byte section name is always returned null-terminated.
 *   9. An RVA whose raw mapping extends outside the file is rejected.
 *
 * @return BOOLEAN TRUE if all tests pass, FALSE if any test fails
 */
BOOLEAN
TestPeParser()
{
    BOOLEAN OverallResult         = TRUE;
    INT32   TestNum               = 0;
    BYTE    Buffer[PeFixtureSize] = {0};

    BuildMinimalPe64(Buffer);
    TestNum++;
    {
        PE_IMAGE_READER Reader = {0};

        if (PeImageReaderInitialize(Buffer, sizeof(Buffer), &Reader) && !PeImageReaderIs32Bit(&Reader))
        {
            printf("[+] Test number %d Passed\n", TestNum);
        }
        else
        {
            printf("[-] Test number %d Failed\n", TestNum);
            printf("[x] valid PE64 did not initialize as PE32+\n");
            return FALSE;
        }
    }

    BuildMinimalPe32(Buffer);
    TestNum++;
    {
        PE_IMAGE_READER Reader = {0};

        if (PeImageReaderInitialize(Buffer, sizeof(Buffer), &Reader) && PeImageReaderIs32Bit(&Reader))
        {
            printf("[+] Test number %d Passed\n", TestNum);
        }
        else
        {
            printf("[-] Test number %d Failed\n", TestNum);
            printf("[x] valid PE32 did not initialize as PE32\n");
            return FALSE;
        }
    }

    BuildMinimalPe64(Buffer);
    WriteWord(Buffer, 0, 0);
    TestNum++;
    {
        PE_IMAGE_READER Reader = {0};

        if (!PeImageReaderInitialize(Buffer, sizeof(Buffer), &Reader))
        {
            printf("[+] Test number %d Passed\n", TestNum);
        }
        else
        {
            printf("[-] Test number %d Failed\n", TestNum);
            printf("[x] invalid DOS magic initialized successfully\n");
            return FALSE;
        }
    }

    BuildMinimalPe64(Buffer);
    WriteWord(Buffer,
              PeHeaderOffset + sizeof(DWORD) + offsetof(IMAGE_FILE_HEADER, SizeOfOptionalHeader),
              sizeof(IMAGE_OPTIONAL_HEADER64) - 1);
    TestNum++;
    {
        PE_IMAGE_READER Reader = {0};

        if (!PeImageReaderInitialize(Buffer, sizeof(Buffer), &Reader))
        {
            printf("[+] Test number %d Passed\n", TestNum);
        }
        else
        {
            printf("[-] Test number %d Failed\n", TestNum);
            printf("[x] truncated optional header initialized successfully\n");
            return FALSE;
        }
    }

    BuildMinimalPe64(Buffer);
    WriteDword(Buffer, offsetof(IMAGE_DOS_HEADER, e_lfanew), PeFixtureSize);
    TestNum++;
    {
        PE_IMAGE_READER Reader = {0};

        if (!PeImageReaderInitialize(Buffer, sizeof(Buffer), &Reader))
        {
            printf("[+] Test number %d Passed\n", TestNum);
        }
        else
        {
            printf("[-] Test number %d Failed\n", TestNum);
            printf("[x] invalid e_lfanew initialized successfully\n");
            return FALSE;
        }
    }

    BuildMinimalPe64(Buffer);
    SetPe64OptionalHeaderSizeOfHeaders(Buffer, 0x1c0);
    ConfigureTextSection(Buffer, sizeof(IMAGE_OPTIONAL_HEADER64), 0x1000, 0x50, 0x1c0, 0x40);
    TestNum++;
    {
        PE_IMAGE_READER Reader     = {0};
        SIZE_T          FileOffset = 0;

        if (PeImageReaderInitialize(Buffer, sizeof(Buffer), &Reader) &&
            PeImageReaderRvaToFileOffset(&Reader, 0x1010, 4, &FileOffset) && FileOffset == 0x1d0)
        {
            printf("[+] Test number %d Passed\n", TestNum);
        }
        else
        {
            printf("[-] Test number %d Failed\n", TestNum);
            printf("[x] valid section RVA did not map to raw file offset\n");
            return FALSE;
        }
    }

    BuildMinimalPe64(Buffer);
    SetPe64OptionalHeaderSizeOfHeaders(Buffer, 0x1c0);
    TestNum++;
    {
        PE_IMAGE_READER Reader     = {0};
        SIZE_T          FileOffset = 0;

        if (PeImageReaderInitialize(Buffer, sizeof(Buffer), &Reader) &&
            PeImageReaderRvaToFileOffset(&Reader, 0x20, 4, &FileOffset) && FileOffset == 0x20 &&
            !PeImageReaderRvaToFileOffset(&Reader, 0x1be, 4, &FileOffset))
        {
            printf("[+] Test number %d Passed\n", TestNum);
        }
        else
        {
            printf("[-] Test number %d Failed\n", TestNum);
            printf("[x] header RVA bounds were not enforced\n");
            return FALSE;
        }
    }

    TestNum++;
    {
        IMAGE_SECTION_HEADER SectionHeader = {0};
        CHAR                 Name[9];

        FillMemory(Name, sizeof(Name), 'X');
        CopyMemory(SectionHeader.Name, "ABCDEFGH", IMAGE_SIZEOF_SHORT_NAME);
        if (PeImageReaderGetSectionName(&SectionHeader, Name, sizeof(Name)) &&
            strcmp(Name, "ABCDEFGH") == 0 && Name[IMAGE_SIZEOF_SHORT_NAME] == '\0')
        {
            printf("[+] Test number %d Passed\n", TestNum);
        }
        else
        {
            printf("[-] Test number %d Failed\n", TestNum);
            printf("[x] 8-byte section name was not null-terminated\n");
            return FALSE;
        }
    }

    BuildMinimalPe64(Buffer);
    SetPe64OptionalHeaderSizeOfHeaders(Buffer, 0x1c0);
    ConfigureTextSection(Buffer, sizeof(IMAGE_OPTIONAL_HEADER64), 0x1000, 0x40, 0x300, 0x20);
    TestNum++;
    {
        PE_IMAGE_READER Reader     = {0};
        SIZE_T          FileOffset = 0;

        if (PeImageReaderInitialize(Buffer, sizeof(Buffer), &Reader) &&
            !PeImageReaderRvaToFileOffset(&Reader, 0x1000, 1, &FileOffset))
        {
            printf("[+] Test number %d Passed\n", TestNum);
        }
        else
        {
            printf("[-] Test number %d Failed\n", TestNum);
            printf("[x] RVA mapping accepted raw pointer outside file\n");
            OverallResult = FALSE;
        }
    }

    return OverallResult;
}
