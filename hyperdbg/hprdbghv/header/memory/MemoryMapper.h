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
 * @brief Different levels of paging
 * 
 */
typedef enum _PAGING_LEVEL
{
    PagingLevelPageTable = 0,
    PagingLevelPageDirectory,
    PagingLevelPageDirectoryPointerTable,
    PagingLevelPageMapLevel4
} PAGING_LEVEL;

/**
 * @brief Memory wrapper for reading safe from the memory
 * 
 */
typedef enum _MEMORY_MAPPER_WRAPPER_FOR_MEMORY_READ
{
    MEMORY_MAPPER_WRAPPER_READ_PHYSICAL_MEMORY,
    MEMORY_MAPPER_WRAPPER_READ_VIRTUAL_MEMORY,
} MEMORY_MAPPER_WRAPPER_FOR_MEMORY_READ;

/**
 * @brief Memory wrapper for writing safe into the memory
 * 
 */
typedef enum _MEMORY_MAPPER_WRAPPER_FOR_MEMORY_WRITE
{
    MEMORY_MAPPER_WRAPPER_WRITE_PHYSICAL_MEMORY,
    MEMORY_MAPPER_WRAPPER_WRITE_VIRTUAL_MEMORY_SAFE,
    MEMORY_MAPPER_WRAPPER_WRITE_VIRTUAL_MEMORY_UNSAFE,

} MEMORY_MAPPER_WRAPPER_FOR_MEMORY_WRITE;

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
        } Fields;
    };
} CR3_TYPE, *PCR3_TYPE;

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
    _In_ MEMORY_MAPPER_WRAPPER_FOR_MEMORY_WRITE TypeOfRead,
    _In_ UINT64                                 AddressToRead);

static BOOLEAN
MemoryMapperReadMemorySafeByPhysicalAddressWrapper(
    _In_ MEMORY_MAPPER_WRAPPER_FOR_MEMORY_WRITE TypeOfRead,
    _In_ UINT64                                 AddressToRead,
    _Inout_ UINT64                              BufferToSaveMemory,
    _In_ SIZE_T                                 SizeToRead);

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

BOOLEAN
MemoryMapperReadMemorySafe(_In_ UINT64   VaAddressToRead,
                           _Inout_ PVOID BufferToSaveMemory,
                           _In_ SIZE_T   SizeToRead);

BOOLEAN
MemoryMapperReadMemorySafeByPhysicalAddress(_In_ UINT64    PaAddressToRead,
                                            _Inout_ UINT64 BufferToSaveMemory,
                                            _In_ SIZE_T    SizeToRead);

BOOLEAN
MemoryMapperReadMemorySafeOnTargetProcess(_In_ UINT64   VaAddressToRead,
                                          _Inout_ PVOID BufferToSaveMemory,
                                          _In_ SIZE_T   SizeToRead);

BOOLEAN
MemoryMapperWriteMemorySafeOnTargetProcess(_Inout_ UINT64 Destination,
                                           _In_ PVOID     Source,
                                           _In_ SIZE_T    Size);

BOOLEAN
MemoryMapperWriteMemorySafe(_Inout_ UINT64 Destination,
                            _In_ PVOID     Source,
                            _In_ SIZE_T    SizeToWrite,
                            _In_ CR3_TYPE  TargetProcessCr3);

PPAGE_ENTRY
MemoryMapperGetPteVa(_In_ PVOID        Va,
                     _In_ PAGING_LEVEL Level);

PPAGE_ENTRY
MemoryMapperGetPteVaByCr3(_In_ PVOID        Va,
                          _In_ PAGING_LEVEL Level,
                          _In_ CR3_TYPE     TargetCr3);

PPAGE_ENTRY
MemoryMapperGetPteVaWithoutSwitchingByCr3(_In_ PVOID        Va,
                                          _In_ PAGING_LEVEL Level,
                                          _In_ CR3_TYPE     TargetCr3);

BOOLEAN
MemoryMapperSetSupervisorBitWithoutSwitchingByCr3(_In_ PVOID        Va,
                                                  _In_ BOOLEAN      Set,
                                                  _In_ PAGING_LEVEL Level,
                                                  _In_ CR3_TYPE     TargetCr3);

BOOLEAN
MemoryMapperCheckIfPageIsPresentByCr3(_In_ PVOID    Va,
                                      _In_ CR3_TYPE TargetCr3);

VOID
MemoryMapperInitialize();

VOID
MemoryMapperUninitialize();

VOID
MemoryMapperMapPhysicalAddressToPte(_In_ PHYSICAL_ADDRESS PhysicalAddress,
                                    _In_ PVOID            TargetProcessVirtualAddress,
                                    _In_ CR3_TYPE         TargetProcessKernelCr3);

UINT64
MemoryMapperReserveUsermodeAddressInTargetProcess(_In_ UINT32  ProcessId,
                                                  _In_ BOOLEAN Allocate);

BOOLEAN
MemoryMapperFreeMemoryInTargetProcess(_In_ UINT32   ProcessId,
                                      _Inout_ PVOID BaseAddress);

BOOLEAN
MemoryMapperWriteMemoryUnsafe(_Inout_ UINT64 Destination,
                              _In_ PVOID     Source,
                              _In_ SIZE_T    SizeToWrite,
                              _In_ UINT32    TargetProcessId);

BOOLEAN
MemoryMapperWriteMemorySafeByPhysicalAddress(_Inout_ UINT64 DestinationPa,
                                             _In_ UINT64    Source,
                                             _In_ SIZE_T    SizeToWrite);

BOOLEAN
MemoryMapperCheckIfPageIsNxBitSetOnTargetProcess(_In_ PVOID Va);
