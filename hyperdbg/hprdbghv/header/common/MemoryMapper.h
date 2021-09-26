/**
 * @file MemoryMapper.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief This file shows the header functions to map memory to reserved system ranges
 * 
 * also some of the functions derived from hvpp
 * - https://github.com/wbenny/hvpp
 * 
 * @version 0.1
 * @date 2020-05-3
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//				    Definitions					//
//////////////////////////////////////////////////

#define PAGE_4KB_OFFSET ((UINT64)(1 << 12) - 1)
#define PAGE_2MB_OFFSET ((UINT64)(1 << 21) - 1)
#define PAGE_4MB_OFFSET ((UINT64)(1 << 22) - 1)
#define PAGE_1GB_OFFSET ((UINT64)(1 << 30) - 1)

//////////////////////////////////////////////////
//					   Enums  					//
//////////////////////////////////////////////////

/**
 * @brief Different levels of paging
 * 
 */
typedef enum _PML
{
    PT = 0, // Page Table
    PD,     // Page Directory
    PDPT,   // Page Directory Pointer Table
    PML4    // Page Map Level 4
} PML;

//////////////////////////////////////////////////
//					Structures					//
//////////////////////////////////////////////////

/**
 * @brief Memory mapper PTE and reserved virtual address
 * 
 */
typedef struct _MEMORY_MAPPER_ADDRESSES
{
    UINT64 PteVirtualAddress; // The virtual address of PTE
    UINT64 VirualAddress;     // The actual kernel virtual address to read or write
} MEMORY_MAPPER_ADDRESSES, *PMEMORY_MAPPER_ADDRESSES;

/**
 * @brief Page Table Entry Structure
 * 
 */
typedef struct _PAGE_TABLE_ENTRY
{
    union
    {
        UINT64 Flags;

        struct
        {
            UINT64 Present : 1;
            UINT64 Write : 1;
            UINT64 Supervisor : 1;
            UINT64 PageLevelWriteThrough : 1;
            UINT64 PageLevelCacheDisable : 1;
            UINT64 Accessed : 1;
            UINT64 Dirty : 1;
            UINT64 Pat : 1;
            UINT64 Global : 1;
            UINT64 Ignored1 : 3;
            UINT64 PageFrameNumber : 36;
            UINT64 Reserved1 : 4;
            UINT64 Ignored2 : 7;
            UINT64 ProtectionKey : 4;
            UINT64 ExecuteDisable : 1;
        };
    };
} PAGE_TABLE_ENTRY, *PPAGE_TABLE_ENTRY;

/**
 * @brief Page Directory Entry Structure
 * 
 */
typedef struct _PAGE_DIRECTORY_ENTRY
{
    union
    {
        UINT64 Flags;

        struct
        {
            UINT64 Present : 1;
            UINT64 Write : 1;
            UINT64 Supervisor : 1;
            UINT64 PageLevelWriteThrough : 1;
            UINT64 PageLevelCacheDisable : 1;
            UINT64 Accessed : 1;
            UINT64 Reserved1 : 1;
            UINT64 LargePage : 1;
            UINT64 Ignored1 : 4;
            UINT64 PageFrameNumber : 36;
            UINT64 Reserved2 : 4;
            UINT64 Ignored2 : 11;
            UINT64 ExecuteDisable : 1;
        };
    };
} PAGE_DIRECTORY_ENTRY, *PPAGE_DIRECTORY_ENTRY;

/**
 * @brief Large Page Directory Entry Structure
 * 
 */
typedef struct _LARGE_PAGE_DIRECTORY_ENTRY
{
    union
    {
        UINT64 Flags;

        struct
        {
            UINT64 Present : 1;
            UINT64 Write : 1;
            UINT64 Supervisor : 1;
            UINT64 PageLevelWriteThrough : 1;
            UINT64 PageLevelCacheDisable : 1;
            UINT64 Accessed : 1;
            UINT64 Dirty : 1;
            UINT64 LargePage : 1;
            UINT64 Global : 1;
            UINT64 Ignored1 : 3;
            UINT64 Pat : 1;
            UINT64 Reserved1 : 17;
            UINT64 PageFrameNumber : 18;
            UINT64 Reserved2 : 4;
            UINT64 Ignored2 : 7;
            UINT64 ProtectionKey : 4;
            UINT64 ExecuteDisable : 1;
        };
    };
} LARGE_PAGE_DIRECTORY_ENTRY, *PLARGE_PAGE_DIRECTORY_ENTRY;

/**
 * @brief Page Directory Pointer Table Entry Structure
 * 
 */
typedef struct _PAGE_DIRECTORY_POINTER_TABLE_ENTRY
{
    union
    {
        UINT64 Flags;

        struct
        {
            UINT64 Present : 1;
            UINT64 Write : 1;
            UINT64 Supervisor : 1;
            UINT64 PageLevelWriteThrough : 1;
            UINT64 PageLevelCacheDisable : 1;
            UINT64 Accessed : 1;
            UINT64 Reserved1 : 1;
            UINT64 LargeLpage : 1;
            UINT64 Ignored1 : 4;
            UINT64 PageFrameNumber : 36;
            UINT64 Reserved2 : 4;
            UINT64 Ignored2 : 11;
            UINT64 ExecuteDisable : 1;
        };
    };
} PAGE_DIRECTORY_POINTER_TABLE_ENTRY, *PPAGE_DIRECTORY_POINTER_TABLE_ENTRY;

/**
 * @brief Large Page Directory Pointer Table Entry Structure
 * 
 */
typedef struct _LARGE_PAGE_DIRECTORY_POINTER_TABLE_ENTRY
{
    union
    {
        UINT64 Flags;

        struct
        {
            UINT64 Present : 1;
            UINT64 Write : 1;
            UINT64 Supervisor : 1;
            UINT64 PageLevelWriteThrough : 1;
            UINT64 PageLevelCacheDisable : 1;
            UINT64 Accessed : 1;
            UINT64 Dirty : 1;
            UINT64 LargePage : 1;
            UINT64 Global : 1;
            UINT64 Ignored1 : 3;
            UINT64 Pat : 1;
            UINT64 Reserved1 : 17;
            UINT64 PageFrameNumber : 18;
            UINT64 Reserved2 : 4;
            UINT64 Ignored2 : 7;
            UINT64 ProtectionKey : 4;
            UINT64 ExecuteDisable : 1;
        };
    };
} LARGE_PAGE_DIRECTORY_POINTER_TABLE_ENTRY, *PLARGE_PAGE_DIRECTORY_POINTER_TABLE_ENTRY;

/**
 * @brief Page Map Level 4 Entry Structure
 * 
 */
typedef struct _PAGE_MAP_LEVEL_4_ENTRY
{
    union
    {
        UINT64 Flags;

        struct
        {
            UINT64 Present : 1;
            UINT64 Write : 1;
            UINT64 Supervisor : 1;
            UINT64 PageLevelWriteThrough : 1;
            UINT64 PageLevelCacheDisable : 1;
            UINT64 Accessed : 1;
            UINT64 Reserved1 : 1;
            UINT64 MustBeZero : 1;
            UINT64 Ignored1 : 4;
            UINT64 PageFrameNumber : 36;
            UINT64 Reserved2 : 4;
            UINT64 Ignored2 : 11;
            UINT64 ExecuteDisable : 1;
        };
    };
} PAGE_MAP_LEVEL_4_ENTRY, *PPAGE_MAP_LEVEL_4_ENTRY;

/**
 * @brief Page Entries
 * 
 */
typedef struct _PAGE_ENTRY
{
    union
    {
        UINT64 Flags;

        PAGE_MAP_LEVEL_4_ENTRY                   Pml4;
        LARGE_PAGE_DIRECTORY_POINTER_TABLE_ENTRY PdptLarge; // 1GB
        PAGE_DIRECTORY_POINTER_TABLE_ENTRY       Pdpt;
        LARGE_PAGE_DIRECTORY_ENTRY               PdLarge; // 2MB
        PAGE_DIRECTORY_ENTRY                     Pd;
        PAGE_TABLE_ENTRY                         Pt;

        //
        // Common fields.
        //

        struct
        {
            UINT64 Present : 1;
            UINT64 Write : 1;
            UINT64 Supervisor : 1;
            UINT64 PageLevelWriteThrough : 1;
            UINT64 PageLevelCacheDisable : 1;
            UINT64 Accessed : 1;
            UINT64 Dirty : 1;
            UINT64 LargePage : 1;
            UINT64 Global : 1;
            UINT64 Ignored1 : 3;
            UINT64 PageFrameNumber : 36;
            UINT64 Reserved1 : 4;
            UINT64 Ignored2 : 7;
            UINT64 ProtectionKey : 4;
            UINT64 ExecuteDisable : 1;
        };
    };
} PAGE_ENTRY, *PPAGE_ENTRY;

/**
 * @brief CR3 Structure
 * 
 */
typedef struct _CR3_TYPE
{
    union
    {
        UINT64 Flags;

        struct
        {
            UINT64 Pcid : 12;
            UINT64 PageFrameNumber : 36;
            UINT64 Reserved1 : 12;
            UINT64 Reserved_2 : 3;
            UINT64 PcidInvalidate : 1;
        };
    };
} CR3_TYPE, *PCR3_TYPE;

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

BOOLEAN
MemoryMapperReadMemorySafe(UINT64 VaAddressToRead, PVOID BufferToSaveMemory, SIZE_T SizeToRead);

BOOLEAN
MemoryMapperReadMemorySafeByPhysicalAddress(UINT64 PaAddressToRead, PVOID BufferToSaveMemory, SIZE_T SizeToRead);

BOOLEAN
MemoryMapperReadMemorySafeOnTargetProcess(UINT64 VaAddressToRead, PVOID BufferToSaveMemory, SIZE_T SizeToRead);

BOOLEAN
MemoryMapperWriteMemorySafeOnTargetProcess(UINT64 Destination, PVOID Source, SIZE_T Size);

BOOLEAN
MemoryMapperWriteMemorySafe(UINT64 Destination, PVOID Source, SIZE_T SizeToRead, CR3_TYPE TargetProcessCr3);

PPAGE_ENTRY
MemoryMapperGetPteVa(PVOID Va, PML Level);

PPAGE_ENTRY
MemoryMapperGetPteVaByCr3(PVOID Va, PML Level, CR3_TYPE TargetCr3);

BOOLEAN
MemoryMapperCheckIfPageIsPresentByCr3(PVOID Va, CR3_TYPE TargetCr3);

VOID
MemoryMapperInitialize();

VOID
MemoryMapperUninitialize();

UINT64
MemoryMapperReserveUsermodeAddressInTargetProcess(UINT32 ProcessId, BOOLEAN Commit);

BOOLEAN
MemoryMapperWriteMemoryUnsafe(UINT64 Destination, PVOID Source, SIZE_T SizeToRead, UINT32 TargetProcessId);

BOOLEAN
MemoryMapperWriteMemorySafeByPhysicalAddress(UINT64 DestinationPa, PVOID Source, SIZE_T SizeToRead);
