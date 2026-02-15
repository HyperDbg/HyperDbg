/**
 * @file Tracing.c
 * @author Hari Mishal (harimishal6@gmail.com)
 * @brief Tracing routines for HyperTrace module
 * @details
 * @version 0.18
 * @date 2025-12-02
 *
 * @copyright This project is released under the GNU Public License v3.
 */

#include "pch.h"
#include "Lbr.h"
/**
 * @brief Hide debugger on transparent-mode (activate transparent-mode)
 *
 * @param HypertraceCallbacks
 *
 * @return BOOLEAN
 */

VOID
PerformLbrTraceAfterEnable()
{
    LBR_IOCTL_REQUEST Request;
    RtlZeroMemory(&Request, sizeof(LBR_IOCTL_REQUEST));
    LbrInitialize();

    // Check if LBR is supported
    // This populates LbrCapacity based on the CPU Model
    if (!LbrCheck())
    {
        LogInfo("LBR is not supported on this CPU model.\n");
        return;
    }
    LogInfo("LBR Support detected. Capacity: %llu entries.\n", LbrCapacity);

    // Enable LBR
    // We set Pid to 0 to target the current process
    Request.LbrConfig.Pid       = 0;
    Request.LbrConfig.LbrSelect = 0; // Uses default LBR_SELECT defined in your headers

    if (LbrEnableLbr(&Request))
    {
        LogInfo("LBR successfully enabled for current process.\n");

        // LbrDumpLbr prints directly to LogInfo inside Lbr.c
        LogInfo("Dumping LBR Buffer:\n");
        LbrDumpLbr(&Request);

        // This cleans up the LBR_STATE and stops the tracing
        if (LbrDisableLbr(&Request))
        {
            LogInfo("LBR successfully disabled.\n");
        }
    }
    else
    {
        LogInfo("Failed to enable LBR.\n");
    }
}

BOOLEAN
HyperTraceInit(HYPERTRACE_CALLBACKS * HypertraceCallbacks)
{
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
    RtlCopyMemory(&g_Callbacks, HypertraceCallbacks, sizeof(HYPERTRACE_CALLBACKS));

    LogInfo("HyperTrace module initialized successfully with provided callbacks.\n");
    PerformLbrTraceAfterEnable();

    return TRUE;
}
