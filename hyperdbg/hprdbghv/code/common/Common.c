/**
 * @file Common.c
 * @author Sina Karvandi (sina@hyperdbg.org)
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
_Use_decl_annotations_
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
_Use_decl_annotations_
UINT64
VirtualAddressToPhysicalAddress(_In_ PVOID VirtualAddress)
{
    return MmGetPhysicalAddress(VirtualAddress).QuadPart;
}

/**
 * @brief Converts pid to kernel cr3
 * 
 * @details this function should NOT be called from vmx-root 
 *
 * @param ProcessId ProcessId to switch
 * @return CR3_TYPE The cr3 of the target process 
 */
_Use_decl_annotations_
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

    ObDereferenceObject(TargetEprocess);

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
_Use_decl_annotations_
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

    ObDereferenceObject(TargetEprocess);

    return CurrentProcessCr3;
}

/**
 * @brief Switch to guest's running process's cr3
 * 
 * @details this function can be called from vmx-root mode
 *
 * @return CR3_TYPE The cr3 of current process which can be
 * used by RestoreToPreviousProcess function
 */
CR3_TYPE
SwitchOnMemoryLayoutOfTargetProcess()
{
    CR3_TYPE GuestCr3;
    CR3_TYPE CurrentProcessCr3 = {0};

    GuestCr3.Flags = GetRunningCr3OnTargetProcess().Flags;

    //
    // Read the current cr3
    //
    CurrentProcessCr3.Flags = __readcr3();

    //
    // Change to a new cr3 (of target process)
    //
    __writecr3(GuestCr3.Flags);

    return CurrentProcessCr3;
}

/**
 * @brief Switch to another process's cr3
 *
 * @param TargetCr3 cr3 to switch
 * @return CR3_TYPE The cr3 of current process which can be
 * used by RestoreToPreviousProcess function
 */
_Use_decl_annotations_
CR3_TYPE
SwitchOnAnotherProcessMemoryLayoutByCr3(CR3_TYPE TargetCr3)
{
    CR3_TYPE CurrentProcessCr3 = {0};

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
_Use_decl_annotations_
BOOLEAN
GetSegmentDescriptor(PUCHAR GdtBase, UINT16 Selector, PVMX_SEGMENT_SELECTOR SegmentSelector)
{
    SEGMENT_DESCRIPTOR_32 * DescriptorTable32;
    SEGMENT_DESCRIPTOR_32 * Descriptor32;
    SEGMENT_SELECTOR        SegSelector = {.AsUInt = Selector};

    if (!SegmentSelector)
        return FALSE;

#define SELECTOR_TABLE_LDT 0x1
#define SELECTOR_TABLE_GDT 0x0

    //
    // Ignore LDT
    //
    if ((Selector == 0x0) || (SegSelector.Table != SELECTOR_TABLE_GDT))
    {
        return FALSE;
    }

    DescriptorTable32 = (SEGMENT_DESCRIPTOR_32 *)(GdtBase);
    Descriptor32      = &DescriptorTable32[SegSelector.Index];

    SegmentSelector->Selector = Selector;
    SegmentSelector->Limit    = __segmentlimit(Selector);
    SegmentSelector->Base     = (Descriptor32->BaseAddressLow | Descriptor32->BaseAddressMiddle << 16 | Descriptor32->BaseAddressHigh << 24);

    SegmentSelector->Attributes.AsUInt = (AsmGetAccessRights(Selector) >> 8);

    if (SegSelector.Table == 0 && SegSelector.Index == 0)
    {
        SegmentSelector->Attributes.Unusable = TRUE;
    }

    if ((Descriptor32->Type == SEGMENT_DESCRIPTOR_TYPE_TSS_BUSY) ||
        (Descriptor32->Type == SEGMENT_DESCRIPTOR_TYPE_CALL_GATE))
    {
        //
        // this is a TSS or callgate etc, save the base high part
        //

        UINT64 SegmentLimitHigh;
        SegmentLimitHigh      = (*(UINT64 *)((PUCHAR)Descriptor32 + 8));
        SegmentSelector->Base = (SegmentSelector->Base & 0xffffffff) | (SegmentLimitHigh << 32);
    }

    if (SegmentSelector->Attributes.Granularity)
    {
        //
        // 4096-bit granularity is enabled for this segment, scale the limit
        //
        SegmentSelector->Limit = (SegmentSelector->Limit << 12) + 0xfff;
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
_Use_decl_annotations_
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
_Use_decl_annotations_
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
_Use_decl_annotations_
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
 * @brief Converts Physical Address to Virtual Address based
 * on current process's kernel cr3
 *   
 * @details this function should NOT be called from vmx-root mode
 *
 * @param PhysicalAddress The target physical address
 * @return UINT64 Returns the virtual address
 */
_Use_decl_annotations_
UINT64
PhysicalAddressToVirtualAddressOnTargetProcess(PVOID PhysicalAddress)
{
    CR3_TYPE GuestCr3;

    GuestCr3.Flags = GetRunningCr3OnTargetProcess().Flags;

    UINT64 Temp = (UINT64)PhysicalAddress;

    return PhysicalAddressToVirtualAddressByCr3(PhysicalAddress, GuestCr3);
}

/**
 * @brief Get cr3 of the target running process
 *   
 * @return CR3_TYPE Returns the cr3 of running process
 */
CR3_TYPE
GetRunningCr3OnTargetProcess()
{
    CR3_TYPE GuestCr3;

    //
    // Due to KVA Shadowing, we need to switch to a different directory table base
    // if the PCID indicates this is a user mode directory table base.
    //
    NT_KPROCESS * CurrentProcess = (NT_KPROCESS *)(PsGetCurrentProcess());
    GuestCr3.Flags               = CurrentProcess->DirectoryTableBase;

    return GuestCr3;
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
_Use_decl_annotations_
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
 * @param VirtualAddress The target virtual address
 * @param TargetCr3 The target's process cr3
 * @return UINT64 Returns the physical address
 */
_Use_decl_annotations_
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
 * @brief Converts Virtual Address to Physical Address based
 * on the current process's kernel cr3
 * 
 * @param VirtualAddress The target virtual address
 * @return UINT64 Returns the physical address
 */
_Use_decl_annotations_
UINT64
VirtualAddressToPhysicalAddressOnTargetProcess(PVOID VirtualAddress)
{
    CR3_TYPE CurrentCr3;
    CR3_TYPE GuestCr3;
    UINT64   PhysicalAddress;

    GuestCr3.Flags = GetRunningCr3OnTargetProcess().Flags;

    //
    // Switch to new process's memory layout
    //
    CurrentCr3 = SwitchOnAnotherProcessMemoryLayoutByCr3(GuestCr3);

    //
    // Validate if process id is valid
    //
    if (CurrentCr3.Flags == NULL)
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
    RestoreToPreviousProcess(CurrentCr3);

    return PhysicalAddress;
}

/**
 * @brief Converts Physical Address to Virtual Address
 * 
 * @param PhysicalAddress The target physical address
 * @return UINT64 Returns the virtual address
 */
_Use_decl_annotations_
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
        ObDereferenceObject(TargetEprocess);

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
 * @brief Get virtual address width for x86 processors
 * 
 * @return UINT32
 */
UINT32
Getx86VirtualAddressWidth()
{
    int Regs[4];

    GetCpuid(CPUID_ADDR_WIDTH, 0, Regs);

    //
    // Extracting bit 15:8 from eax register
    //
    return ((Regs[0] >> 8) & 0x0ff);
}

/**
 * @brief Checks if the address is canonical based on x86 processor's
 * virtual address width or not
 * @param VAddr virtual address to check
 * @param IsKernelAddress Filled to show whether the address is a 
 * kernel address or user-address
 * @brief IsKernelAddress wouldn't check for page attributes, it 
 * just checks the address convention in Windows
 * 
 * @return BOOLEAN
 */
BOOLEAN
CheckCanonicalVirtualAddress(UINT64 VAddr, PBOOLEAN IsKernelAddress)
{
    UINT64 Addr = (UINT64)VAddr;
    UINT64 MaxVirtualAddrLowHalf, MinVirtualAddressHighHalf;

    //
    // Get processor's address width for VA
    //
    UINT32 AddrWidth = g_VirtualAddressWidth;

    //
    // get max address in lower-half canonical addr space
    // e.g. if width is 48, then 0x00007FFF_FFFFFFFF
    //
    MaxVirtualAddrLowHalf = ((UINT64)1ull << (AddrWidth - 1)) - 1;

    //
    // get min address in higher-half canonical addr space
    // e.g., if width is 48, then 0xFFFF8000_00000000
    //
    MinVirtualAddressHighHalf = ~MaxVirtualAddrLowHalf;

    //
    // Check to see if the address in a canonical address
    //
    if ((Addr > MaxVirtualAddrLowHalf) && (Addr < MinVirtualAddressHighHalf))
    {
        *IsKernelAddress = FALSE;
        return FALSE;
    }

    //
    // Set whether it's a kernel address or not
    //
    if (MinVirtualAddressHighHalf < Addr)
    {
        *IsKernelAddress = TRUE;
    }
    else
    {
        *IsKernelAddress = FALSE;
    }

    return TRUE;
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
    BOOLEAN  IsKernelAddress;
    BOOLEAN  Result = FALSE;

    //
    // First, we check if the address is canonical based
    // on Intel processor's virtual address width
    //
    if (!CheckCanonicalVirtualAddress(TargetAddress, &IsKernelAddress))
    {
        //
        // No need for further check, address is invalid
        //
        Result = FALSE;
        goto Return;
    }

    //
    // Find the current process cr3
    //
    GuestCr3.Flags = GetRunningCr3OnTargetProcess().Flags;

    //
    // Move to new cr3
    //
    OriginalCr3 = __readcr3();
    __writecr3(GuestCr3.Flags);

    //
    // We'll only check address with TSX if the address is a kernel-mode
    // address because an exception is thrown if we access user-mode code
    // from vmx-root mode, thus, TSX will fail the transaction and the
    // result is not true, so we check each pages' page-table for user-mode
    // codes in both user-mode and kernel-mode
    //
    // if (g_RtmSupport && IsKernelAddress)
    // {
    //     //
    //     // The guest supports Intel TSX
    //     //
    //     UINT64 AlignedPage = (UINT64)PAGE_ALIGN(TargetAddress);
    //     UINT64 PageCount   = ((TargetAddress - AlignedPage) + Size) / PAGE_SIZE;
    //
    //     for (size_t i = 0; i <= PageCount; i++)
    //     {
    //         UINT64 CheckAddr = AlignedPage + (PAGE_SIZE * i);
    //         if (!CheckIfAddressIsValidUsingTsx(CheckAddr))
    //         {
    //             //
    //             // Address is not valid
    //             //
    //             Result = FALSE;
    //
    //             goto RestoreCr3;
    //         }
    //     }
    // }

    //
    // We've realized that using TSX for address checking is not faster
    // than the legacy memory checking (traversing the page-tables),
    // based on our resultsm it's ~50 TSC clock cycles for a valid address
    // and ~180 TSC clock cycles for an invalid address slower to use TSX
    // for memory checking, that's why it is deprecated now
    //

    //
    // Check if memory is safe and present
    //
    UINT64 AddressToCheck =
        (CHAR *)TargetAddress + Size - ((CHAR *)PAGE_ALIGN(TargetAddress));

    if (AddressToCheck > PAGE_SIZE)
    {
        //
        // Address should be accessed in more than one page
        //
        UINT64 ReadSize = AddressToCheck;

        while (Size != 0)
        {
            ReadSize = (UINT64)PAGE_ALIGN(TargetAddress + PAGE_SIZE) - TargetAddress;

            if (ReadSize == PAGE_SIZE && Size < PAGE_SIZE)
            {
                ReadSize = Size;
            }

            if (!MemoryMapperCheckIfPageIsPresentByCr3(TargetAddress, GuestCr3))
            {
                //
                // Address is not valid
                //
                Result = FALSE;

                goto RestoreCr3;
            }

            /*
            LogInfo("Addr From : %llx to Addr To : %llx | ReadSize : %llx\n",
                    TargetAddress,
                    TargetAddress + ReadSize,
                    ReadSize);
            */

            //
            // Apply the changes to the next addresses (if any)
            //
            Size          = Size - ReadSize;
            TargetAddress = TargetAddress + ReadSize;
        }
    }
    else
    {
        if (!MemoryMapperCheckIfPageIsPresentByCr3(TargetAddress, GuestCr3))
        {
            //
            // Address is not valid
            //
            Result = FALSE;

            goto RestoreCr3;
        }
    }

    //
    // If we've reached here, the address was valid
    //
    Result = TRUE;

RestoreCr3:

    //
    // Move back to original cr3
    //
    __writecr3(OriginalCr3);

Return:
    return Result;
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
    GuestCr3.Flags = GetRunningCr3OnTargetProcess().Flags;

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
    GuestCr3.Flags = GetRunningCr3OnTargetProcess().Flags;

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

/**
 * @brief Allocates a buffer and tests for the MSRs that cause #GP
 * 
 * @return UINT64 Allocated buffer for MSR Bitmap
 */
UINT64 *
AllocateInvalidMsrBimap()
{
    UINT64 * InvalidMsrBitmap;

    InvalidMsrBitmap = ExAllocatePoolWithTag(NonPagedPool, 0x1000 / 0x8, POOLTAG);

    if (InvalidMsrBitmap == NULL)
    {
        return NULL;
    }

    RtlZeroMemory(InvalidMsrBitmap, 0x1000 / 0x8);

    for (size_t i = 0; i < 0x1000; ++i)
    {
        __try
        {
            __readmsr(i);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            SetBit(i, InvalidMsrBitmap);
        }
    }

    return InvalidMsrBitmap;
}

/**
 * @brief Get handle from Process Id
 * @param Handle
 * @param ProcessId
 * 
 * @return NTSTATUS 
 */
_Use_decl_annotations_
NTSTATUS
GetHandleFromProcess(UINT32 ProcessId, PHANDLE Handle)
{
    NTSTATUS Status;
    Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES ObjAttr;
    CLIENT_ID         Cid;
    InitializeObjectAttributes(&ObjAttr, NULL, 0, NULL, NULL);

    Cid.UniqueProcess = ProcessId;
    Cid.UniqueThread  = (HANDLE)0;

    Status = ZwOpenProcess(Handle, PROCESS_ALL_ACCESS, &ObjAttr, &Cid);

    return Status;
}

/**
 * @brief The undocumented way of NtOpenProcess
 * @param ProcessHandle
 * @param DesiredAccess
 * @param ProcessId
 * @param AccessMode
 * 
 * @return NTSTATUS 
 */
NTSTATUS
UndocumentedNtOpenProcess(
    PHANDLE         ProcessHandle,
    ACCESS_MASK     DesiredAccess,
    HANDLE          ProcessId,
    KPROCESSOR_MODE AccessMode)
{
    NTSTATUS     status = STATUS_SUCCESS;
    ACCESS_STATE accessState;
    char         auxData[0x200];
    PEPROCESS    processObject = NULL;
    HANDLE       processHandle = NULL;

    status = SeCreateAccessState(
        &accessState,
        auxData,
        DesiredAccess,
        (PGENERIC_MAPPING)((PCHAR)*PsProcessType + 52));

    if (!NT_SUCCESS(status))
        return status;

    accessState.PreviouslyGrantedAccess |= accessState.RemainingDesiredAccess;
    accessState.RemainingDesiredAccess = 0;

    status = PsLookupProcessByProcessId(ProcessId, &processObject);

    if (!NT_SUCCESS(status))
    {
        SeDeleteAccessState(&accessState);
        return status;
    }
    status = ObOpenObjectByPointer(
        processObject,
        0,
        &accessState,
        0,
        *PsProcessType,
        AccessMode,
        &processHandle);

    SeDeleteAccessState(&accessState);

    ObDereferenceObject(processObject);

    if (NT_SUCCESS(status))
        *ProcessHandle = processHandle;

    return status;
}

/**
 * @brief Kill a user-mode process with different methods
 * @param ProcessId
 * @param KillingMethod
 * 
 * @return BOOLEAN 
 */
_Use_decl_annotations_
BOOLEAN
KillProcess(UINT32 ProcessId, PROCESS_KILL_METHODS KillingMethod)
{
    NTSTATUS  Status        = STATUS_SUCCESS;
    HANDLE    ProcessHandle = NULL;
    PEPROCESS Process       = NULL;

    if (ProcessId == NULL)
    {
        return FALSE;
    }

    switch (KillingMethod)
    {
    case PROCESS_KILL_METHOD_1:

        Status = GetHandleFromProcess(ProcessId, &ProcessHandle);

        if (!NT_SUCCESS(Status) || ProcessHandle == NULL)
        {
            return FALSE;
        }

        //
        // Call ZwTerminateProcess with NULL handle
        //
        Status = ZwTerminateProcess(ProcessHandle, 0);

        if (!NT_SUCCESS(Status))
        {
            return FALSE;
        }

        break;

    case PROCESS_KILL_METHOD_2:

        UndocumentedNtOpenProcess(
            &ProcessHandle,
            PROCESS_ALL_ACCESS,
            ProcessId,
            KernelMode);

        if (ProcessHandle == NULL)
        {
            return FALSE;
        }

        //
        // Call ZwTerminateProcess with NULL handle
        //
        Status = ZwTerminateProcess(ProcessHandle, 0);

        if (!NT_SUCCESS(Status))
        {
            return FALSE;
        }

        break;

    case PROCESS_KILL_METHOD_3:

        //
        // Get the base address of process's executable image and unmap it
        //
        Status = MmUnmapViewOfSection(Process, PsGetProcessSectionBaseAddress(Process));

        //
        // Dereference the target process
        //
        ObDereferenceObject(Process);

        break;

    default:

        //
        // Unknow killing method
        //
        return FALSE;
        break;
    }

    //
    // If we reached here, it means the functionality of
    // the above codes was successful
    //
    return TRUE;
}
