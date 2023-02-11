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
 * @return VOID
 */
BOOLEAN
DirtyLoggingInitialize()
{

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

    //
    // The PML address and PML index fields exist only on processors that support the 1-setting of
    // the "enable PML" VM - execution control
    //
    ULONG SecondaryProcBasedVmExecControls = HvAdjustControls(IA32_VMX_PROCBASED_CTLS2_ENABLE_PML_FLAG, IA32_VMX_PROCBASED_CTLS2);

    if (SecondaryProcBasedVmExecControls & IA32_VMX_PROCBASED_CTLS2_ENABLE_PML_FLAG == 0x0) {
        LogWarning("err, dirty logging mechanism is not initialized as the processor doesn't support PML");
        return FALSE;
    }

    //
    // A new 64-bit VM-execution control field is defined called the PML address. This is
    // the 4 - KByte aligned physical address of the page - modification log.The page modification
    // log comprises 512 64 - bit entries
    //
    if (g_PmlBufferAddress == NULL) {
        g_PmlBufferAddress = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOLTAG);
    }

    if (g_PmlBufferAddress == NULL) {
        return FALSE;
    }

    //
    // Clear the log buffer
    //
    RtlZeroBytes(g_PmlBufferAddress, PAGE_SIZE);

    //
    // Call the VMCALL to adjust PML controls from vmx-root
    //
    AsmVmxVmcall(VMCALL_ENABLE_DIRTY_LOGGING_MECHANISM, 0, 0, 0);

    //
    // Initialization was successful
    //
    return TRUE;
}

/**
 * @brief Enables the dirty logging mechanism in VMX-root mode
 * @details should be called in vmx-root mode
 *
 * @return VOID
 */
VOID DirtyLoggingEnable()
{
    //
    // Write the address of the buffer
    //
    LogInfo("PML Buffer Address = %llx", VirtualAddressToPhysicalAddress(g_PmlBufferAddress));
    __vmx_vmwrite(VMCS_CTRL_PML_ADDRESS, VirtualAddressToPhysicalAddress(g_PmlBufferAddress));

    //
    // Clear the PML index
    //
    __vmx_vmwrite(VMCS_GUEST_PML_INDEX, 0x0);

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
}

/**
 * @brief Disables the dirty logging mechanism in VMX-root mode
 * @details should be called in vmx-root mode
 *
 * @return VOID
 */
VOID DirtyLoggingDisable()
{
}

/**
 * @brief Initialize the dirty logging mechanism
 *
 * @return VOID
 */
BOOLEAN
DirtyLoggingUninitialize()
{
}

VOID DirtyLoggingHandlePageModificationLog()
{
    //
    // The guest-physical address of the access is written to the page-modification log
    //
    for (size_t i = 0; i < PAGE_SIZE / sizeof(UINT64); i++) {

        LogInfo("Address : %llx", g_PmlBufferAddress[i]);
    }
}

/**
 * @brief Handling vm-exits of PML
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID DirtyLoggingHandleVmexits(VIRTUAL_MACHINE_STATE* VCpu)
{
    LogInfo("Dirty Logging VM-exit");

    //
    // *** The guest-physical address of the access is written to the page-modification log
    // and the buffer is full ***
    //
    DirtyLoggingHandlePageModificationLog();

    //
    // Do not increment RIP
    //
    HvSuppressRipIncrement(VCpu);
}
