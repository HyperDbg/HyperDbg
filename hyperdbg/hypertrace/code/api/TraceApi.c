/**
 * @file TraceApi.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Tracing routines for HyperTrace module
 * @details
 * @version 0.19
 * @date 2026-04-25
 *
 * @copyright This project is released under the GNU Public License v3.
 */
#include "pch.h"

/**
 * @brief Initialize the hyper trace module callbacks
 * @details This only for callback initialization, not for LBR, PT, etc. initialization
 *
 * @param HypertraceCallbacks
 * @param RunningOnHypervisorEnvironment Whether the initialization is being done for hypervisor environment or not,
 * it can be used to skip some of the initialization steps if it is not for hypervisor environment and behave differently based on that
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceInitCallback(HYPERTRACE_CALLBACKS * HypertraceCallbacks,
                       BOOLEAN                RunningOnHypervisorEnvironment)
{
    UINT32 ProcessorsCount = 0;

    //
    // Check if any of the required callbacks are NULL
    //
    for (UINT32 i = 0; i < sizeof(HYPERTRACE_CALLBACKS) / sizeof(UINT64); i++)
    {
        if (((PVOID *)HypertraceCallbacks)[i] == NULL)
        {
            //
            // The callback has null entry, so we cannot proceed
            //
            return FALSE;
        }
    }

    //
    // Save the callbacks
    //
    PlatformWriteMemory(&g_Callbacks, HypertraceCallbacks, sizeof(HYPERTRACE_CALLBACKS));

    //
    // Query the number of processors in the system to initialize the global LBR state list accordingly
    //
    ProcessorsCount = PlatformCpuGetActiveProcessorCount();

    //
    // Initialize the global LBR state list to hold LBR states for each core
    //
    g_LbrStateList = (LBR_STACK_ENTRY *)PlatformMemAllocateZeroedNonPagedPool(sizeof(LBR_STACK_ENTRY) * ProcessorsCount);

    //
    // Initialize the global PT per-CPU state list. Each entry starts in
    // PT_STATE_DISABLED with no buffers allocated; PtStart() will lazily
    // allocate ToPA / output / overflow buffers on first use per core.
    //
    g_PtStateList = (PT_PER_CPU *)PlatformMemAllocateZeroedNonPagedPool(sizeof(PT_PER_CPU) * ProcessorsCount);
    if (g_PtStateList != NULL)
    {
        UINT32 i;
        for (i = 0; i < ProcessorsCount; i++)
        {
            PtEngineInitDefaultConfig(&g_PtStateList[i].Config);
            g_PtStateList[i].State = PT_STATE_DISABLED;
        }
    }

    //
    // Set the flag to indicate whether the initialization is being done for hypervisor environment or not
    //
    g_RunningOnHypervisorEnvironment = RunningOnHypervisorEnvironment;

    //
    // It is initialized, but LBR is disabled at this stage
    //
    g_LastBranchRecordEnabled = FALSE;

    //
    // It is initialized, but Processor Trace is disabled at this stage
    //
    g_ProcessorTraceEnabled = FALSE;

    //
    // Enable callbacks and set the initialized flag
    //
    g_HyperTraceCallbacksInitialized = TRUE;

    return TRUE;
}

/**
 * @brief Uninitialize the hypertrace module
 *
 * @return VOID
 */
VOID
HyperTraceUnInit()
{
    //
    // Disable LBR tracing if it is still enabled
    //
    if (g_LastBranchRecordEnabled)
    {
        HyperTraceLbrDisable(NULL);
    }

    //
    // Unallocate the global LBR state list if it is allocated
    //
    if (g_LbrStateList != NULL)
    {
        PlatformMemFreePool(g_LbrStateList);
        g_LbrStateList = NULL;
    }

    //
    // Disable Processor Trace if it is still enabled
    //
    if (g_ProcessorTraceEnabled)
    {
        HyperTracePtDisable(NULL);
    }

    //
    // Free PT buffers (if any) and the per-CPU state list
    //
    if (g_PtStateList != NULL)
    {
        UINT32 ProcessorsCountLocal = KeQueryActiveProcessorCount(0);
        UINT32 i;

        for (i = 0; i < ProcessorsCountLocal; i++)
        {
            PtEngineFreeBuffers(&g_PtStateList[i]);
        }

        PlatformMemFreePool(g_PtStateList);
        g_PtStateList = NULL;
    }

    //
    // Reset the environment flag to default value
    //
    g_RunningOnHypervisorEnvironment = FALSE;

    //
    // Set callbacks to not initialized
    //
    g_HyperTraceCallbacksInitialized = FALSE;
}
