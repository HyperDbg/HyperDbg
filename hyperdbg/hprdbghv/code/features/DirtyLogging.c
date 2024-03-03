/**
 * @file DirtyLogging.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of memory hooks functions
 * @details
 *
 * @version 0.2
 * @date 2023-02-05
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Initialize the dirty logging mechanism
 *
 * @return BOOLEAN
 */
BOOLEAN
DirtyLoggingInitialize()
{
    ULONG ProcessorsCount;

    //
    // Query count of active processors
    //
    ProcessorsCount = KeQueryActiveProcessorCount(0);

    //
    // The explanations are copied from Intel whitepaper on PML:
    // Link : https://www.intel.com/content/dam/www/public/us/en/documents/white-papers/page-modification-logging-vmm-white-paper.pdf
    //

    //
    // When PML is active each write that sets a dirty flag for EPT also generates an entry in an inmemory log,
    // reporting the guest-physical address of the write (aligned to 4 KBytes)
    // When the log is full, a VM exit occurs, notifying the VMM.A VMM can monitor the number of pages modified
    //  by each thread by specifying an available set of log entries
    //

    //
    // Check for the support of PML
    //
    if (!g_CompatibilityCheck.PmlSupport)
    {
        LogWarning("err, dirty logging mechanism is not initialized as the processor doesn't support PML");
        return FALSE;
    }

    //
    // A new 64-bit VM-execution control field is defined called the PML address. This is
    // the 4 - KByte aligned physical address of the page - modification log.The page modification
    // log comprises 512 64 - bit entries
    //
    for (size_t i = 0; i < ProcessorsCount; i++)
    {
        if (g_GuestState[i].PmlBufferAddress == NULL)
        {
            g_GuestState[i].PmlBufferAddress = CrsAllocateNonPagedPool(PAGE_SIZE);
        }

        if (g_GuestState[i].PmlBufferAddress == NULL)
        {
            //
            // Allocation failed
            //
            for (size_t j = 0; j < ProcessorsCount; j++)
            {
                if (g_GuestState[j].PmlBufferAddress != NULL)
                {
                    CrsFreePool(g_GuestState[j].PmlBufferAddress);
                }
            }

            return FALSE;
        }

        //
        // Clear the log buffer
        //
        RtlZeroBytes(g_GuestState[i].PmlBufferAddress, PAGE_SIZE);
    }

    //
    // Broadcast VMCALL to adjust PML controls from vmx-root
    //
    BroadcastEnablePmlOnAllProcessors();

    //
    // Initialization was successful
    //
    return TRUE;
}

/**
 * @brief Enables the dirty logging mechanism in VMX-root mode
 * @details should be called in vmx-root mode
 *
 * @param VCpu The virtual processor's state
 *
 * @return BOOLEAN
 */
BOOLEAN
DirtyLoggingEnable(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Check if PML Address buffer is empty or not
    //
    if (VCpu->PmlBufferAddress == NULL)
    {
        return FALSE;
    }

    //
    // Write the address of the buffer
    //
    UINT64 PmlPhysAddr = VirtualAddressToPhysicalAddress(VCpu->PmlBufferAddress);

    // LogInfo("PML Buffer Address = %llx", PmlPhysAddr);

    __vmx_vmwrite(VMCS_CTRL_PML_ADDRESS, PmlPhysAddr);

    //
    // Clear the PML index
    //
    __vmx_vmwrite(VMCS_GUEST_PML_INDEX, PML_ENTITY_NUM - 1);

    //
    // If the "enable PML" VM-execution control is 1 and bit 6 of EPT pointer (EPTP)
    //  is 1 (enabling accessed and dirty flags for EPT) then we can use this feature
    // by default we set this flag for EPTP so, we have to enable PML ENABLE Bit in the
    // secondary proc-based controls
    //

    //
    // Secondary processor-based VM-execution control 17 is defined as enable PML
    //
    HvSetPmlEnableFlag(TRUE);

    //
    // Initialization was successful
    //
    return TRUE;
}

/**
 * @brief Disables the dirty logging mechanism in VMX-root mode
 * @details should be called in vmx-root mode
 *
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
DirtyLoggingDisable(VIRTUAL_MACHINE_STATE * VCpu)
{
    UNREFERENCED_PARAMETER(VCpu);

    //
    // Clear the address
    //
    __vmx_vmwrite(VMCS_CTRL_PML_ADDRESS, NULL64_ZERO);

    //
    // Clear the PML index
    //
    __vmx_vmwrite(VMCS_GUEST_PML_INDEX, 0x0);

    //
    // Disable PML Enable bit
    //
    HvSetPmlEnableFlag(FALSE);
}

/**
 * @brief Initialize the dirty logging mechanism
 *
 * @return VOID
 */
VOID
DirtyLoggingUninitialize()
{
    ULONG ProcessorsCount;

    //
    // Query count of active processors
    //
    ProcessorsCount = KeQueryActiveProcessorCount(0);

    //
    // Broadcast VMCALL to disable PML controls from vmx-root
    //
    BroadcastDisablePmlOnAllProcessors();

    //
    // Free the allocated pool buffers
    //
    for (size_t i = 0; i < ProcessorsCount; i++)
    {
        if (g_GuestState[i].PmlBufferAddress != NULL)
        {
            CrsFreePool(g_GuestState[i].PmlBufferAddress);
        }
    }
}

/**
 * @brief Create log from dirty log buffer
 *
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
DirtyLoggingHandlePageModificationLog(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // The guest-physical address of the access is written to the page-modification log
    //
    for (size_t i = 0; i < PML_ENTITY_NUM; i++)
    {
        LogInfo("Address : %llx", VCpu->PmlBufferAddress[i]);
    }
}

BOOLEAN
DirtyLoggingFlushPmlBuffer(VIRTUAL_MACHINE_STATE * VCpu)
{
    UINT64 * PmlBuf;
    UINT16   PmlIdx;
    BOOLEAN  IsLargePage;
    PVOID    PmlEntry;

    VmxVmread16P(VMCS_GUEST_PML_INDEX, &PmlIdx);

    //
    // Do nothing if PML buffer is empty
    //
    if (PmlIdx == (PML_ENTITY_NUM - 1))
        return FALSE;

    //
    // PML index always points to next available PML buffer entity
    //
    if (PmlIdx >= PML_ENTITY_NUM)
    {
        PmlIdx = 0;
    }
    else
    {
        PmlIdx++;
    }

    PmlBuf = VCpu->PmlBufferAddress;

    for (; PmlIdx < PML_ENTITY_NUM; PmlIdx++)
    {
        UINT64 AccessedPhysAddr;

        AccessedPhysAddr = PmlBuf[PmlIdx];

        PmlEntry = EptGetPml1OrPml2Entry(VCpu->EptPageTable, AccessedPhysAddr, &IsLargePage);

        if (PmlEntry == NULL)
        {
            //
            // Page PML entry is not valid
            //
            LogInfo("Err, null page");
            continue;
        }

        if (IsLargePage)
        {
            ((PEPT_PML2_ENTRY)PmlEntry)->Dirty = FALSE;
        }
        else
        {
            ((PEPT_PML1_ENTRY)PmlEntry)->Dirty = FALSE;
        }
    }

    //
    // reset PML index
    //
    __vmx_vmwrite(VMCS_GUEST_PML_INDEX, PML_ENTITY_NUM - 1);

    return TRUE;
}

/**
 * @brief Handling vm-exits of PML
 *
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
DirtyLoggingHandleVmexits(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // *** The guest-physical address of the access is written
    // to the page-modification log and the buffer is full ***
    //

    //
    // Test log
    //
    LogInfo("Dirty Logging VM-exit");
    // DirtyLoggingHandlePageModificationLog(VCpu);

    //
    // Flush the PML buffer
    //
    DirtyLoggingFlushPmlBuffer(VCpu);

    //
    // Do not increment RIP
    //
    HvSuppressRipIncrement(VCpu);
}
