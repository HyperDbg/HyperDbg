/**
 * @file Tracing.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers for the tracing functions
 * @details
 * @version 0.7
 * @date 2023-11-03
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

VOID
TracingHandleMtfVmexit(VIRTUAL_MACHINE_STATE * VCpu);

VOID
TracingRestoreSystemState(VIRTUAL_MACHINE_STATE * VCpu);

VOID
TracingCheckForContinuingSteps(VIRTUAL_MACHINE_STATE * VCpu);
