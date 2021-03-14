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
#include "pch.h"

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
 * @brief Check whether the bit is set or not
 * 
 * @param nth 
 * @param addr 
 * @return int 
 */
int
TestBit(int nth, unsigned long * addr)
{
    return (BITMAP_ENTRY(nth, addr) >> BITMAP_SHIFT(nth)) & 1;
}

/**
 * @brief unset the bit
 * 
 * @param nth 
 * @param addr 
 */
void
ClearBit(int nth, unsigned long * addr)
{
    BITMAP_ENTRY(nth, addr) &= ~(1UL << BITMAP_SHIFT(nth));
}

/**
 * @brief set the bit
 * 
 * @param nth 
 * @param addr 
 */
void
SetBit(int nth, unsigned long * addr)
{
    BITMAP_ENTRY(nth, addr) |= (1UL << BITMAP_SHIFT(nth));
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
 * @brief Converts pid to kernel cr3
 * 
 * @details this function should NOT be called from vmx-root mode
 *
 * @param ProcessId ProcessId to switch
 * @return CR3_TYPE The cr3 of the target process 
 */
CR3_TYPE
GetCr3FromProcessId(UINT32 ProcessId)
{
    PEPROCESS TargetEprocess;
    CR3_TYPE  ProcessCr3 = {0};

    if (PsLookupProcessByProcessId(ProcessId, &TargetEprocess) != STATUS_SUCCESS)
    {
        //
        // There was an error, probably the process id was not found
        //
        return ProcessCr3;
    }

    //
    // Due to KVA Shadowing, we need to switch to a different directory table base
    // if the PCID indicates this is a user mode directory table base.
    //
    NT_KPROCESS * CurrentProcess = (NT_KPROCESS *)(TargetEprocess);
    ProcessCr3.Flags             = CurrentProcess->DirectoryTableBase;

    return ProcessCr3;
}

/**
 * @brief Switch to another process's cr3
 * 
 * @details this function should NOT be called from vmx-root mode
 *
 * @param ProcessId ProcessId to switch
 * @return CR3_TYPE The cr3 of current process which can be
 * used by RestoreToPreviousProcess function
 */
CR3_TYPE
SwitchOnAnotherProcessMemoryLayout(UINT32 ProcessId)
{
    UINT64    GuestCr3;
    PEPROCESS TargetEprocess;
    CR3_TYPE  CurrentProcessCr3 = {0};

    if (PsLookupProcessByProcessId(ProcessId, &TargetEprocess) != STATUS_SUCCESS)
    {
        //
        // There was an error, probably the process id was not found
        //
        return CurrentProcessCr3;
    }

    //
    // Due to KVA Shadowing, we need to switch to a different directory table base
    // if the PCID indicates this is a user mode directory table base.
    //
    NT_KPROCESS * CurrentProcess = (NT_KPROCESS *)(TargetEprocess);
    GuestCr3                     = CurrentProcess->DirectoryTableBase;

    //
    // Read the current cr3
    //
    CurrentProcessCr3.Flags = __readcr3();

    //
    // Change to a new cr3 (of target process)
    //
    __writecr3(GuestCr3);

    return CurrentProcessCr3;
}

/**
 * @brief Switch to another process's cr3
 * 
 * @param TargetCr3 cr3 to switch
 * @return CR3_TYPE The cr3 of current process which can be
 * used by RestoreToPreviousProcess function
 */
CR3_TYPE
SwitchOnAnotherProcessMemoryLayoutByCr3(CR3_TYPE TargetCr3)
{
    PEPROCESS TargetEprocess;
    CR3_TYPE  CurrentProcessCr3 = {0};

    //
    // Read the current cr3
    //
    CurrentProcessCr3.Flags = __readcr3();

    //
    // Change to a new cr3 (of target process)
    //
    __writecr3(TargetCr3.Flags);

    return CurrentProcessCr3;
}

/**
 * @brief Get Segment Descriptor
 * 
 * @param SegmentSelector 
 * @param Selector 
 * @param GdtBase 
 * @return BOOLEAN 
 */
BOOLEAN
GetSegmentDescriptor(PSEGMENT_SELECTOR SegmentSelector, USHORT Selector, PUCHAR GdtBase)
{
    PSEGMENT_DESCRIPTOR SegDesc;

    if (!SegmentSelector)
        return FALSE;

    if (Selector & 0x4)
    {
        return FALSE;
    }

    SegDesc = (PSEGMENT_DESCRIPTOR)((PUCHAR)GdtBase + (Selector & ~0x7));

    SegmentSelector->SEL               = Selector;
    SegmentSelector->BASE              = SegDesc->BASE0 | SegDesc->BASE1 << 16 | SegDesc->BASE2 << 24;
    SegmentSelector->LIMIT             = SegDesc->LIMIT0 | (SegDesc->LIMIT1ATTR1 & 0xf) << 16;
    SegmentSelector->ATTRIBUTES.UCHARs = SegDesc->ATTR0 | (SegDesc->LIMIT1ATTR1 & 0xf0) << 4;

    if (!(SegDesc->ATTR0 & 0x10))
    {
        //
        // LA_ACCESSED
        //
        ULONG64 tmp;

        //
        // this is a TSS or callgate etc, save the base high part
        //
        tmp                   = (*(PULONG64)((PUCHAR)SegDesc + 8));
        SegmentSelector->BASE = (SegmentSelector->BASE & 0xffffffff) | (tmp << 32);
    }

    if (SegmentSelector->ATTRIBUTES.Fields.G)
    {
        //
        // 4096-bit granularity is enabled for this segment, scale the limit
        //
        SegmentSelector->LIMIT = (SegmentSelector->LIMIT << 12) + 0xfff;
    }

    return TRUE;
}

/**
 * @brief Switch to previous process's cr3
 * 
 * @param PreviousProcess Cr3 of previous process which 
 * is returned by SwitchOnAnotherProcessMemoryLayout
 * @return VOID 
 */
VOID
RestoreToPreviousProcess(CR3_TYPE PreviousProcess)
{
    //
    // Restore the original cr3
    //
    __writecr3(PreviousProcess.Flags);
}

/**
 * @brief Converts Physical Address to Virtual Address based
 * on a specific process id
 *   
 * @details this function should NOT be called from vmx-root mode
 *
 * @param PhysicalAddress The target physical address
 * @param ProcessId The target's process id
 * @return UINT64 Returns the virtual address
 */
UINT64
PhysicalAddressToVirtualAddressByProcessId(PVOID PhysicalAddress, UINT32 ProcessId)
{
    CR3_TYPE         CurrentProcessCr3;
    UINT64           VirtualAddress;
    PHYSICAL_ADDRESS PhysicalAddr;

    //
    // Switch to new process's memory layout
    //
    CurrentProcessCr3 = SwitchOnAnotherProcessMemoryLayout(ProcessId);

    //
    // Validate if process id is valid
    //
    if (CurrentProcessCr3.Flags == NULL)
    {
        //
        // Pid is invalid
        //
        return NULL;
    }

    //
    // Read the virtual address based on new cr3
    //
    PhysicalAddr.QuadPart = PhysicalAddress;
    VirtualAddress        = MmGetVirtualForPhysical(PhysicalAddr);

    //
    // Restore the original process
    //
    RestoreToPreviousProcess(CurrentProcessCr3);

    return VirtualAddress;
}

/**
 * @brief Converts Physical Address to Virtual Address based
 * on a specific process's kernel cr3
 *   
 * @details this function should NOT be called from vmx-root mode
 *
 * @param PhysicalAddress The target physical address
 * @param TargetCr3 The target's process cr3
 * @return UINT64 Returns the virtual address
 */
UINT64
PhysicalAddressToVirtualAddressByCr3(PVOID PhysicalAddress, CR3_TYPE TargetCr3)
{
    CR3_TYPE         CurrentProcessCr3;
    UINT64           VirtualAddress;
    PHYSICAL_ADDRESS PhysicalAddr;

    //
    // Switch to new process's memory layout
    //
    CurrentProcessCr3 = SwitchOnAnotherProcessMemoryLayoutByCr3(TargetCr3);

    //
    // Validate if process id is valid
    //
    if (CurrentProcessCr3.Flags == NULL)
    {
        //
        // Pid is invalid
        //
        return NULL;
    }

    //
    // Read the virtual address based on new cr3
    //
    PhysicalAddr.QuadPart = PhysicalAddress;
    VirtualAddress        = MmGetVirtualForPhysical(PhysicalAddr);

    //
    // Restore the original process
    //
    RestoreToPreviousProcess(CurrentProcessCr3);

    return VirtualAddress;
}

/**
 * @brief Converts Virtual Address to Physical Address based
 * on a specific process id's kernel cr3
 *
 * @details this function should NOT be called from vmx-root mode
 * 
 * @param VirtualAddress The target virtual address
 * @param ProcessId The target's process id
 * @return UINT64 Returns the physical address
 */
UINT64
VirtualAddressToPhysicalAddressByProcessId(PVOID VirtualAddress, UINT32 ProcessId)
{
    CR3_TYPE CurrentProcessCr3;
    UINT64   PhysicalAddress;

    //
    // Switch to new process's memory layout
    //
    CurrentProcessCr3 = SwitchOnAnotherProcessMemoryLayout(ProcessId);

    //
    // Validate if process id is valid
    //
    if (CurrentProcessCr3.Flags == NULL)
    {
        //
        // Pid is invalid
        //
        return NULL;
    }

    //
    // Read the physical address based on new cr3
    //
    PhysicalAddress = MmGetPhysicalAddress(VirtualAddress).QuadPart;

    //
    // Restore the original process
    //
    RestoreToPreviousProcess(CurrentProcessCr3);

    return PhysicalAddress;
}

/**
 * @brief Converts Virtual Address to Physical Address based
 * on a specific process's kernel cr3
 *
 * @details this function should NOT be called from vmx-root mode
 * 
 * @param VirtualAddress The target virtual address
 * @param TargetCr3 The target's process cr3
 * @return UINT64 Returns the physical address
 */
UINT64
VirtualAddressToPhysicalAddressByProcessCr3(PVOID VirtualAddress, CR3_TYPE TargetCr3)
{
    CR3_TYPE CurrentProcessCr3;
    UINT64   PhysicalAddress;

    //
    // Switch to new process's memory layout
    //
    CurrentProcessCr3 = SwitchOnAnotherProcessMemoryLayoutByCr3(TargetCr3);

    //
    // Validate if process id is valid
    //
    if (CurrentProcessCr3.Flags == NULL)
    {
        //
        // Pid is invalid
        //
        return NULL;
    }

    //
    // Read the physical address based on new cr3
    //
    PhysicalAddress = MmGetPhysicalAddress(VirtualAddress).QuadPart;

    //
    // Restore the original process
    //
    RestoreToPreviousProcess(CurrentProcessCr3);

    return PhysicalAddress;
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

/**
 * @brief Get process name by eprocess 
 * 
 * @param Eprocess Process eprocess
 * @return PCHAR Returns a pointer to the process name
 */
PCHAR
GetProcessNameFromEprocess(PEPROCESS Eprocess)
{
    PCHAR Result = 0;

    //
    // We can't use PsLookupProcessByProcessId as in pageable and not
    // work on vmx-root
    //
    Result = (CHAR *)PsGetProcessImageFileName(Eprocess);

    return Result;
}

/**
 * @brief Detects whether the string starts with another string
 * 
 * @param const char * pre
 * @param const char * str
 * @return BOOLEAN Returns true if it starts with and false if not strats with
 */
BOOLEAN
StartsWith(const char * pre, const char * str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? FALSE : memcmp(pre, str, lenpre) == 0;
}

/**
 * @brief Checks whether the process with ProcId exists or not
 * 
 * @details this function should NOT be called from vmx-root mode
 *
 * @param UINT32 ProcId
 * @return BOOLEAN Returns true if the process 
 * exists and false if it the process doesn't exist
 */
BOOLEAN
IsProcessExist(UINT32 ProcId)
{
    PEPROCESS TargetEprocess;
    CR3_TYPE  CurrentProcessCr3 = {0};

    if (PsLookupProcessByProcessId(ProcId, &TargetEprocess) != STATUS_SUCCESS)
    {
        //
        // There was an error, probably the process id was not found
        //
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/**
 * @brief This function checks whether the address is valid or not using 
 * Intel TSX
 * 
 * @param Address Address to check
 *
 * @param UINT32 ProcId
 * @return BOOLEAN Returns true if the address is valid; otherwise, false
 */
BOOLEAN
CheckIfAddressIsValidUsingTsx(CHAR * Address)
{
    UINT32  Status = 0;
    BOOLEAN Result = FALSE;
    CHAR    TempContent;

    if ((Status = _xbegin()) == _XBEGIN_STARTED)
    {
        //
        // Try to read the memory
        //
        TempContent = *(CHAR *)Address;
        _xend();

        //
        // No error, address is valid
        //
        Result = TRUE;
    }
    else
    {
        //
        // Address is not valid, it aborts the tsx rtm
        //
        Result = FALSE;
    }

    return Result;
}

/**
 * @brief Get cpuid results
 * 
 * @param UINT32 Func
 * @param UINT32 SubFunc
 * @param int * CpuInfo
 * @return VOID
 */
VOID
GetCpuid(UINT32 Func, UINT32 SubFunc, int * CpuInfo)
{
    __cpuidex(CpuInfo, Func, SubFunc);
}

/**
 * @brief Check whether the processor supports RTM or not
 *
 * @return BOOLEAN
 */
BOOLEAN
CheckCpuSupportRtm()
{
    int     Regs1[4];
    int     Regs2[4];
    BOOLEAN Result;

    //
    // TSX is controlled via MSR_IA32_TSX_CTRL.  However, support for this
    // MSR is enumerated by ARCH_CAP_TSX_MSR bit in MSR_IA32_ARCH_CAPABILITIES.
    //
    // TSX control (aka MSR_IA32_TSX_CTRL) is only available after a
    // microcode update on CPUs that have their MSR_IA32_ARCH_CAPABILITIES
    // bit MDS_NO=1. CPUs with MDS_NO=0 are not planned to get
    // MSR_IA32_TSX_CTRL support even after a microcode update. Thus,
    // tsx= cmdline requests will do nothing on CPUs without
    // MSR_IA32_TSX_CTRL support.
    //

    GetCpuid(0, 0, Regs1);
    GetCpuid(7, 0, Regs2);

    //
    // Check RTM and MPX extensions in order to filter out TSX on Haswell CPUs
    //
    Result = Regs1[0] >= 0x7 && (Regs2[1] & 0x4800) == 0x4800;

    return Result;
}

/**
 * @brief Check the safety to access the memory
 * @param TargetAddress
 * @param Size
 * 
 * @return BOOLEAN
 */
BOOLEAN
CheckMemoryAccessSafety(UINT64 TargetAddress, UINT32 Size)
{
    CR3_TYPE GuestCr3;
    UINT64   OriginalCr3;

    //
    // Find the current process cr3
    //
    NT_KPROCESS * CurrentProcess = (NT_KPROCESS *)(PsGetCurrentProcess());
    GuestCr3.Flags               = CurrentProcess->DirectoryTableBase;

    //
    // Move to new cr3
    //
    OriginalCr3 = __readcr3();
    __writecr3(GuestCr3.Flags);

    if (g_RtmSupport)
    {
        //
        // The guest supports Intel TSX
        //
        UINT64 AlignedPage = (UINT64)PAGE_ALIGN(TargetAddress);
        UINT64 PageCount   = ((TargetAddress - AlignedPage) + Size) / PAGE_SIZE;

        for (size_t i = 0; i <= PageCount; i++)
        {
            UINT64 CheckAddr = AlignedPage + (PAGE_SIZE * i);
            if (!CheckIfAddressIsValidUsingTsx(CheckAddr))
            {
                //
                // Move back to original cr3
                //
                __writecr3(OriginalCr3);

                //
                // Access failed and Result = FALSE
                //

                return FALSE;
            }
        }
    }
    else
    {
        //
        // The guest not supports Intel TSX
        //

        //
        // Check if memory is safe and present
        //
        UINT64 AlignedPage = (UINT64)PAGE_ALIGN(TargetAddress);
        UINT64 PageCount   = ((TargetAddress - AlignedPage) + Size) / PAGE_SIZE;

        for (size_t i = 0; i <= PageCount; i++)
        {
            UINT64 CheckAddr = AlignedPage + (PAGE_SIZE * i);
            if (!MemoryMapperCheckIfPageIsPresentByCr3(CheckAddr, GuestCr3))
            {
                //
                // Move back to original cr3
                //
                __writecr3(OriginalCr3);

                return FALSE;
            }
        }
    }

    //
    // Move back to original cr3
    //
    __writecr3(OriginalCr3);

    return TRUE;
}

/**
 * @brief implementation of vmx-root mode compatible strlen
 * @param S
 * 
 * @return UINT32 If 0x0 indicates an error, otherwise length of the 
 * string
 */
UINT32
VmxrootCompatibleStrlen(const CHAR * S)
{
    CHAR     Temp  = NULL;
    UINT32   Count = 0;
    UINT64   AlignedAddress;
    CR3_TYPE GuestCr3;
    CR3_TYPE OriginalCr3;

    AlignedAddress = (UINT64)PAGE_ALIGN((UINT64)S);

    //
    // Find the current process cr3
    //
    NT_KPROCESS * CurrentProcess = (NT_KPROCESS *)(PsGetCurrentProcess());
    GuestCr3.Flags               = CurrentProcess->DirectoryTableBase;

    //
    // Move to new cr3
    //
    OriginalCr3.Flags = __readcr3();
    __writecr3(GuestCr3.Flags);

    //
    // First check
    //
    if (!CheckMemoryAccessSafety(AlignedAddress, sizeof(CHAR)))
    {
        //
        // Error
        //

        //
        // Move back to original cr3
        //
        __writecr3(OriginalCr3.Flags);
        return 0;
    }

    while (TRUE)
    {
        /*
        Temp = *S;
        */
        MemoryMapperReadMemorySafe(S, &Temp, sizeof(CHAR));

        if (Temp != '\0')
        {
            Count++;
            S++;
        }
        else
        {
            //
            // Move back to original cr3
            //
            __writecr3(OriginalCr3.Flags);
            return Count;
        }

        if (!((UINT64)S & (PAGE_SIZE - 1)))
        {
            if (!CheckMemoryAccessSafety((UINT64)S, sizeof(CHAR)))
            {
                //
                // Error
                //

                //
                // Move back to original cr3
                //
                __writecr3(OriginalCr3.Flags);
                return 0;
            }
        }
    }

    //
    // Move back to original cr3
    //
    __writecr3(OriginalCr3.Flags);
}

/**
 * @brief implementation of vmx-root mode compatible wcslen
 * @param S
 * 
 * @return UINT32 If 0x0 indicates an error, otherwise length of the 
 * string
 */
UINT32
VmxrootCompatibleWcslen(const wchar_t * S)
{
    wchar_t  Temp  = NULL;
    UINT32   Count = 0;
    UINT64   AlignedAddress;
    CR3_TYPE GuestCr3;
    CR3_TYPE OriginalCr3;

    AlignedAddress = (UINT64)PAGE_ALIGN((UINT64)S);

    //
    // Find the current process cr3
    //
    NT_KPROCESS * CurrentProcess = (NT_KPROCESS *)(PsGetCurrentProcess());
    GuestCr3.Flags               = CurrentProcess->DirectoryTableBase;

    //
    // Move to new cr3
    //
    OriginalCr3.Flags = __readcr3();
    __writecr3(GuestCr3.Flags);

    AlignedAddress = (UINT64)PAGE_ALIGN((UINT64)S);

    //
    // First check
    //
    if (!CheckMemoryAccessSafety(AlignedAddress, sizeof(wchar_t)))
    {
        //
        // Error
        //

        //
        // Move back to original cr3
        //
        __writecr3(OriginalCr3.Flags);
        return 0;
    }

    while (TRUE)
    {
        /*
        Temp = *S;
        */
        MemoryMapperReadMemorySafe(S, &Temp, sizeof(wchar_t));

        if (Temp != '\0\0')
        {
            Count++;
            S++;
        }
        else
        {
            //
            // Move back to original cr3
            //
            __writecr3(OriginalCr3.Flags);
            return Count;
        }

        if (!((UINT64)S & (PAGE_SIZE - 1)))
        {
            if (!CheckMemoryAccessSafety((UINT64)S, sizeof(wchar_t)))
            {
                //
                // Error
                //

                //
                // Move back to original cr3
                //
                __writecr3(OriginalCr3.Flags);
                return 0;
            }
        }
    }

    //
    // Move back to original cr3
    //
    __writecr3(OriginalCr3.Flags);
}
