/**
 * @file test-pe-parser.cpp
 * @author jtaw5649
 * @brief Test cases for PE parser helpers
 * @details
 * @version 0.11
 * @date 2026-06-01
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#include "header/pe-image-reader.h"

static constexpr SIZE_T PeFixtureSize  = 0x200;
static constexpr LONG   PeHeaderOffset = 0x80;

static SIZE_T
PeOptionalHeaderOffset()
{
    return PeHeaderOffset + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER);
}

static SIZE_T
PeSectionHeaderOffset(SIZE_T OptionalHeaderSize)
{
    return PeOptionalHeaderOffset() + OptionalHeaderSize;
}

static VOID
WriteWord(BYTE * Buffer, SIZE_T Offset, WORD Value)
{
    Buffer[Offset]     = (BYTE)(Value & 0xff);
    Buffer[Offset + 1] = (BYTE)((Value >> 8) & 0xff);
}

static VOID
WriteDword(BYTE * Buffer, SIZE_T Offset, DWORD Value)
{
    Buffer[Offset]     = (BYTE)(Value & 0xff);
    Buffer[Offset + 1] = (BYTE)((Value >> 8) & 0xff);
    Buffer[Offset + 2] = (BYTE)((Value >> 16) & 0xff);
    Buffer[Offset + 3] = (BYTE)((Value >> 24) & 0xff);
}

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

static VOID
SetPe64OptionalHeaderSizeOfHeaders(BYTE * Buffer, DWORD SizeOfHeaders)
{
    SIZE_T Offset = PeOptionalHeaderOffset() + offsetof(IMAGE_OPTIONAL_HEADER64, SizeOfHeaders);

    WriteDword(Buffer, Offset, SizeOfHeaders);
}

static IMAGE_SECTION_HEADER *
GetFixtureSectionHeader(BYTE * Buffer, SIZE_T OptionalHeaderSize)
{
    return (IMAGE_SECTION_HEADER *)(Buffer + PeSectionHeaderOffset(OptionalHeaderSize));
}

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
