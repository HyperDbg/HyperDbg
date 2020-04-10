/**
 * @file Common.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Common functions that needs to be used in all source code files
 * @details
 * @version 0.1
 * @date 2020-04-10
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include <ntddk.h>
#include <wdf.h>
#include "Msr.h"
#include "Common.h"
#include "Vmx.h"

/**
 * @brief Power function in order to computer address for MSR bitmaps
 * 
 * @param Base Base for power function
 * @param Exp Exponent for power function
 * @return int Returns the result of power function
 */
int
MathPower(int Base, int Exp)
{
    int result;

    result = 1;

    for (;;)
    {
        if (Exp & 1)
        {
            result *= Base;
        }
        Exp >>= 1;
        if (!Exp)
        {
            break;
        }
        Base *= Base;
    }
    return result;
}

/**
 * @brief Broadcast a function to all logical cores
 * @details This function is deprecated as we want to supporrt more than 32 processors
 * 
 * @param ProcessorNumber The logical core number to execute routine on it
 * @param Routine The fucntion that should be executed on the target core
 * @return BOOLEAN Returns true if it was successfull
 */
BOOLEAN
BroadcastToProcessors(ULONG ProcessorNumber, RunOnLogicalCoreFunc Routine)
{
    KIRQL OldIrql;

    KeSetSystemAffinityThread((KAFFINITY)(1 << ProcessorNumber));

    OldIrql = KeRaiseIrqlToDpcLevel();

    Routine(ProcessorNumber);

    KeLowerIrql(OldIrql);

    KeRevertToUserAffinityThread();

    return TRUE;
}
/**
 * @brief Set Bits for a special address (used on MSR Bitmaps)
 * 
 * @param Addr Address to the byte
 * @param bit Bit address
 * @param Set Set or Unset
 */
void
SetBit(PVOID Addr, UINT64 bit, BOOLEAN Set)
{
    UINT64 byte;
    UINT64 temp;
    UINT64 n;
    BYTE * Addr2;

    byte = bit / 8;
    temp = bit % 8;
    n    = 7 - temp;

    Addr2 = Addr;

    if (Set)
    {
        Addr2[byte] |= (1 << n);
    }
    else
    {
        Addr2[byte] &= ~(1 << n);
    }
}

/**
 * @brief Get Bits of a special address (used on MSR Bitmaps)
 * 
 * @param Addr Address to the byte
 * @param bit Bit address
 * @return BOOLEAN Returns whther the bit is set or unset
 */
BOOLEAN
GetBit(PVOID Addr, UINT64 bit)
{
    UINT64 byte, k;
    BYTE * Addr2;

    byte = 0;
    k    = 0;
    byte = bit / 8;
    k    = 7 - bit % 8;

    Addr2 = Addr;

    return Addr2[byte] & (1 << k);
}

/**
 * @brief Converts Virtual Address to Physical Address
 * 
 * @param VirtualAddress The target virtual address
 * @return UINT64 Returns the physical address
 */
UINT64
VirtualAddressToPhysicalAddress(PVOID VirtualAddress)
{
    return MmGetPhysicalAddress(VirtualAddress).QuadPart;
}

/**
 * @brief Converts Physical Address to Virtual Address
 * 
 * @param PhysicalAddress The target physical address
 * @return UINT64 Returns the virtual address
 */
UINT64
PhysicalAddressToVirtualAddress(UINT64 PhysicalAddress)
{
    PHYSICAL_ADDRESS PhysicalAddr;
    PhysicalAddr.QuadPart = PhysicalAddress;

    return MmGetVirtualForPhysical(PhysicalAddr);
}

/**
 * @brief Find cr3 of system process
 * 
 * @return UINT64 Returns cr3 of System process (pid=4)
 */
UINT64
FindSystemDirectoryTableBase()
{   
    //
    // Return CR3 of the system process.
    //
    NT_KPROCESS * SystemProcess = (NT_KPROCESS *)(PsInitialSystemProcess);
    return SystemProcess->DirectoryTableBase;
}
