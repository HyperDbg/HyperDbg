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

    KAFFINITY Affinity = 1;
    KeSetSystemAffinityThread(Affinity);

    LbrInitialize();

    if (!LbrCheck())
        return;

    Request.LbrConfig.Pid       = 0;
    Request.LbrConfig.LbrSelect = LBR_SELECT;

    if (LbrEnableLbr(&Request))
    {
        for (volatile int i = 0; i < 50; i++)
        {
            if (i % 2)
            {
                int a = i * 2;
                a += 5;
            }
            else
            {
                __nop();
                __nop();
            }
        }
        LBR_STATE * State = LbrFindLbrState(0);
        if (State)
            LbrGetLbr(State);

        LogInfo("Dumping LBR Buffer...\n");
        LbrDumpLbr(&Request);

        LbrDisableLbr(&Request);
    }

    KeRevertToUserAffinityThread();
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
