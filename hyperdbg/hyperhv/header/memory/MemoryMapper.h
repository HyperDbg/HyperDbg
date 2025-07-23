/**
 * @file MemoryMapper.c
 * @author Sina Karvandi (sina@hyperdbg.org)
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
 * @brief Memory wrapper for reading safe from the memory
 *
 */
typedef enum _MEMORY_MAPPER_WRAPPER_FOR_MEMORY_READ
{
    MEMORY_MAPPER_WRAPPER_READ_PHYSICAL_MEMORY,
    MEMORY_MAPPER_WRAPPER_READ_VIRTUAL_MEMORY,
    MEMORY_MAPPER_WRAPPER_READ_VIRTUAL_MEMORY_UNSAFE

} MEMORY_MAPPER_WRAPPER_FOR_MEMORY_READ;

/**
 * @brief Memory wrapper for writing safe into the memory
 *
 */
typedef enum _MEMORY_MAPPER_WRAPPER_FOR_MEMORY_WRITE
{
    MEMORY_MAPPER_WRAPPER_WRITE_PHYSICAL_MEMORY,
    MEMORY_MAPPER_WRAPPER_WRITE_VIRTUAL_MEMORY_SAFE,
    MEMORY_MAPPER_WRAPPER_WRITE_VIRTUAL_MEMORY_UNSAFE

} MEMORY_MAPPER_WRAPPER_FOR_MEMORY_WRITE;

//////////////////////////////////////////////////
//					Structures					//
//////////////////////////////////////////////////

/**
 * @brief Page Entries
 *
 */
typedef struct _PAGE_ENTRY
{
    union
    {
        UINT64 Flags;

        PML4E_64     Pml4;
        PDPTE_1GB_64 PdptLarge; // 1GB
        PDPTE_64     Pdpt;
        PDE_2MB_64   PdLarge; // 2MB
        PDE_64       Pd;
        PTE_64       Pt;

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
        } Fields;
    };
} PAGE_ENTRY, *PPAGE_ENTRY;

/**
 * @brief Memory mapper PTE and reserved virtual address
 * @details Memory mapper details for each core, contains PTE Virtual Address, Actual Kernel Virtual Address
 */
typedef struct _MEMORY_MAPPER_ADDRESSES
{
    UINT64 PteVirtualAddressForRead; // The virtual address of PTE for read operations
    UINT64 VirualAddressForRead;     // The actual kernel virtual address to read

    UINT64 PteVirtualAddressForWrite; // The virtual address of PTE for write operations
    UINT64 VirualAddressForWrite;     // The actual kernel virtual address to write
} MEMORY_MAPPER_ADDRESSES, *PMEMORY_MAPPER_ADDRESSES;

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// Private Interfaces
//

static UINT64
MemoryMapperGetIndex(_In_ PAGING_LEVEL Level,
                     _In_ UINT64       Va);

static UINT32
MemoryMapperGetOffset(_In_ PAGING_LEVEL Level,
                      _In_ UINT64       Va);

static BOOLEAN
MemoryMapperCheckIfPageIsNxBitSetByCr3(_In_ PVOID    Va,
                                       _In_ CR3_TYPE TargetCr3);

static PVOID
MemoryMapperMapReservedPageRange(_In_ SIZE_T Size);

static VOID
MemoryMapperUnmapReservedPageRange(_In_ PVOID VirtualAddress);

static PVOID
MemoryMapperGetPte(_In_ PVOID VirtualAddress);

static PVOID
MemoryMapperGetPteByCr3(_In_ PVOID    VirtualAddress,
                        _In_ CR3_TYPE TargetCr3);

static PVOID
MemoryMapperMapPageAndGetPte(_Out_ PUINT64 PteAddress);

static BOOLEAN
MemoryMapperReadMemorySafeByPte(_In_ PHYSICAL_ADDRESS PaAddressToRead,
                                _Inout_ PVOID         BufferToSaveMemory,
                                _In_ SIZE_T           SizeToRead,
                                _In_ UINT64           PteVaAddress,
                                _In_ UINT64           MappingVa,
                                _In_ BOOLEAN          InvalidateVpids);

static BOOLEAN
MemoryMapperWriteMemorySafeByPte(_In_ PVOID            SourceVA,
                                 _In_ PHYSICAL_ADDRESS PaAddressToWrite,
                                 _In_ SIZE_T           SizeToWrite,
                                 _Inout_ UINT64        PteVaAddress,
                                 _Inout_ UINT64        MappingVa,
                                 _In_ BOOLEAN          InvalidateVpids);

static UINT64
MemoryMapperReadMemorySafeByPhysicalAddressWrapperAddressMaker(
    _In_ MEMORY_MAPPER_WRAPPER_FOR_MEMORY_READ TypeOfRead,
    _In_ UINT64                                AddressToRead,
    _In_ UINT32                                TargetProcessId);

static BOOLEAN
MemoryMapperReadMemorySafeWrapper(
    _In_ MEMORY_MAPPER_WRAPPER_FOR_MEMORY_READ TypeOfRead,
    _In_ UINT64                                AddressToRead,
    _Inout_ UINT64                             BufferToSaveMemory,
    _In_ SIZE_T                                SizeToRead,
    _In_ UINT32                                TargetProcessId);

static UINT64
MemoryMapperWriteMemorySafeWrapperAddressMaker(_In_ MEMORY_MAPPER_WRAPPER_FOR_MEMORY_WRITE TypeOfWrite,
                                               _In_ UINT64                                 DestinationAddr,
                                               _In_opt_ PCR3_TYPE                          TargetProcessCr3,
                                               _In_opt_ UINT32                             TargetProcessId);

static BOOLEAN
MemoryMapperWriteMemorySafeWrapper(_In_ MEMORY_MAPPER_WRAPPER_FOR_MEMORY_WRITE TypeOfWrite,
                                   _Inout_ UINT64                              DestinationAddr,
                                   _In_ UINT64                                 Source,
                                   _In_ SIZE_T                                 SizeToWrite,
                                   _In_opt_ PCR3_TYPE                          TargetProcessCr3,
                                   _In_opt_ UINT32                             TargetProcessId);

// ----------------------------------------------------------------------------
// Public Interfaces
//

VOID
MemoryMapperInitialize();

VOID
MemoryMapperUninitialize();

BOOLEAN
MemoryMapperCheckIfPageIsPresentByCr3(_In_ PVOID    Va,
                                      _In_ CR3_TYPE TargetCr3);

VOID
MemoryMapperMapPhysicalAddressToPte(_In_ PHYSICAL_ADDRESS PhysicalAddress,
                                    _In_ PVOID            TargetProcessVirtualAddress,
                                    _In_ CR3_TYPE         TargetProcessKernelCr3);
