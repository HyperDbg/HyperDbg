/**
 * @file DirtyLogging.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers for dirty logging mechanism
 * @details
 * @version 0.2
 * @date 2023-02-12
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				   Constants					//
//////////////////////////////////////////////////

#define PML_ENTITY_NUM 512

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

BOOLEAN
DirtyLoggingInitialize();

BOOLEAN
DirtyLoggingEnable(VIRTUAL_MACHINE_STATE * VCpu);

VOID
DirtyLoggingDisable(VIRTUAL_MACHINE_STATE * VCpu);

VOID
DirtyLoggingUninitialize();

VOID
DirtyLoggingHandleVmexits(VIRTUAL_MACHINE_STATE * VCpu);
