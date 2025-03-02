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
 * @brief Broadcast a function to all logical cores
 * @details This function is deprecated as we want to supporrt more than 32 processors
 *
 * @param ProcessorNumber The logical core number to execute routine on it
 * @param Routine The function that should be executed on the target core
 * @return BOOLEAN Returns true if it was successful
 */
_Use_decl_annotations_
BOOLEAN
CommonAffinityBroadcastToProcessors(ULONG ProcessorNumber, RunOnLogicalCoreFunc Routine)
{
    KIRQL OldIrql;

    KeSetSystemAffinityThread((KAFFINITY)(1ULL << ProcessorNumber));

    OldIrql = KeRaiseIrqlToDpcLevel();

    Routine(ProcessorNumber);

    KeLowerIrql(OldIrql);

    KeRevertToUserAffinityThread();

    return TRUE;
}

/**
 * @brief Get process name by eprocess
 *
 * @param Eprocess Process eprocess
 * @return PCHAR Returns a pointer to the process name
 */
PCHAR
CommonGetProcessNameFromProcessControlBlock(PEPROCESS Eprocess)
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
CommonIsStringStartsWith(const char * pre, const char * str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? FALSE : memcmp(pre, str, lenpre) == 0;
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
CommonCpuidInstruction(UINT32 Func, UINT32 SubFunc, int * CpuInfo)
{
    __cpuidex(CpuInfo, Func, SubFunc);
}

/**
 * @brief determines if the guest was in 32-bit user-mode or 64-bit (long mode)
 * @details this function should be called from vmx-root
 *
 * @return BOOLEAN
 */
BOOLEAN
CommonIsGuestOnUsermode32Bit()
{
    //
    // Only 16 bit is needed however, VMWRITE might write on other bits
    // and corrupt other variables, that's why we get 64bit
    //
    UINT64 CsSel = NULL64_ZERO;

    //
    // Read guest's cs selector
    //
    CsSel = HvGetCsSelector();

    if (CsSel == KGDT64_R0_CODE)
    {
        //
        // 64-bit kernel-mode
        //
        return FALSE;
    }
    else if ((CsSel & ~3) == KGDT64_R3_CODE)
    {
        //
        // 64-bit user-mode
        //
        return FALSE;
    }
    else if ((CsSel & ~3) == KGDT64_R3_CMCODE)
    {
        //
        // 32-bit user-mode
        //
        return TRUE;
    }
    else if ((CsSel & ~3) == KGDT64_R0_CMCODE)
    {
        //
        // 32-bit kernel-mode
        //
        return TRUE;
    }
    else
    {
        LogError("Err, unknown value for cs, cannot determine wow64 mode");
    }

    //
    // By default, 64-bit
    //
    return FALSE;
}

/**
 * @brief Produce debug information from unrecoverable bugs
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
CommonWriteDebugInformation(VIRTUAL_MACHINE_STATE * VCpu)
{
    LogError("HyperDbg cannot recover from this error, please provide the following information through the Git issues");

    LogInfo("Target RIP: %llx\n", VCpu->LastVmexitRip);

    CHAR Instruction[MAXIMUM_INSTR_SIZE] = {0};

    MemoryMapperReadMemorySafeOnTargetProcess(VCpu->LastVmexitRip, Instruction, MAXIMUM_INSTR_SIZE);

    for (size_t i = 0; i < MAXIMUM_INSTR_SIZE; i++)
    {
        Log("%02X ", Instruction[i] & 0xffU);
    }

    Log("\n");
    DisassemblerShowOneInstructionInVmxRootMode((PVOID)VCpu->LastVmexitRip,
                                                CommonIsGuestOnUsermode32Bit());
    Log("\n");

    Log(
        "RAX=%016llx RBX=%016llx RCX=%016llx\n"
        "RDX=%016llx RSI=% 016llx RDI=%016llx\n"
        "RIP=%016llx RSP=%016llx RBP=%016llx\n"
        "R8 =%016llx R9 =%016llx R10=%016llx\n"
        "R11=%016llx R12=%016llx R13=%016llx\n"
        "R14=%016llx R15=%016llx\n",
        VCpu->Regs->rax,
        VCpu->Regs->rbx,
        VCpu->Regs->rcx,
        VCpu->Regs->rdx,
        VCpu->Regs->rsi,
        VCpu->Regs->rdi,
        VCpu->LastVmexitRip,
        VCpu->Regs->rsp,
        VCpu->Regs->rbp,
        VCpu->Regs->r8,
        VCpu->Regs->r9,
        VCpu->Regs->r10,
        VCpu->Regs->r11,
        VCpu->Regs->r12,
        VCpu->Regs->r13,
        VCpu->Regs->r14,
        VCpu->Regs->r15);
}

/**
 * @brief Check correctness of the XCR0 register value.
 * @param XCr0 The XCR0 register value.
 *
 * @return BOOLEAN
 */
BOOLEAN
CommonIsXCr0Valid(XCR0 XCr0)
{
    CPUID_EAX_0D_ECX_00 XCr0CpuidInfo;
    UINT64              UnsupportedXCr0Bits;

    //
    // SDM Vol.3A 2.6 EXTENDED CONTROL REGISTERS (INCLUDING XCR0) - describes the invalid
    // states of the XCR0 register.
    //

    CommonCpuidInstruction(CPUID_EXTENDED_STATE_INFORMATION, 0, (INT32 *)&XCr0CpuidInfo);
    UnsupportedXCr0Bits = ~(XCr0CpuidInfo.Eax.AsUInt | (UINT64)XCr0CpuidInfo.Edx.AsUInt << 32);

    if ((XCr0.AsUInt & UnsupportedXCr0Bits) != 0)
    {
        return FALSE;
    }

    if (XCr0.X87 != 1)
    {
        return FALSE;
    }

    if (XCr0.Sse == 0 && XCr0.Avx == 1)
    {
        return FALSE;
    }

    if (XCr0.Avx == 0)
    {
        if ((XCr0.Opmask | XCr0.ZmmHi256 | XCr0.ZmmHi16) == 1)
        {
            return FALSE;
        }
    }

    if (XCr0.Bndcsr != XCr0.Bndreg)
    {
        return FALSE;
    }

    if ((XCr0.Opmask | XCr0.ZmmHi256 | XCr0.ZmmHi16) == 1)
    {
        if ((XCr0.Opmask & XCr0.ZmmHi256 & XCr0.ZmmHi16) != 1)
        {
            return FALSE;
        }
    }

    return TRUE;
}
