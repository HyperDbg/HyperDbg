/**
 * @file PtApi.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for PT tracing routines for HyperTrace module (Intel Processor Trace)
 * @details
 * @version 0.19
 * @date 2026-04-29
 *
 * @copyright This project is released under the GNU Public License v3.
 */
#pragma once

//////////////////////////////////////////////////
//                  Functions                   //
//////////////////////////////////////////////////

BOOLEAN
HyperTracePtEnable(HYPERTRACE_PT_OPERATION_PACKETS * PtOperationRequest);

BOOLEAN
HyperTracePtPause(HYPERTRACE_PT_OPERATION_PACKETS * HyperTraceOperationRequest);

BOOLEAN
HyperTracePtResume(HYPERTRACE_PT_OPERATION_PACKETS * HyperTraceOperationRequest);

BOOLEAN
HyperTracePtSize(HYPERTRACE_PT_OPERATION_PACKETS * HyperTraceOperationRequest);

BOOLEAN
HyperTracePtDump(HYPERTRACE_PT_OPERATION_PACKETS * HyperTraceOperationRequest);

BOOLEAN
HyperTracePtFlush(HYPERTRACE_PT_OPERATION_PACKETS * HyperTraceOperationRequest);

BOOLEAN
HyperTracePtFilter(HYPERTRACE_PT_OPERATION_PACKETS * HyperTraceOperationRequest);

BOOLEAN
HyperTracePtMmap(HYPERTRACE_PT_MMAP_PACKETS * Req);
