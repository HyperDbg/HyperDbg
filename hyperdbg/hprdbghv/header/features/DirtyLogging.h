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
//				   Functions					//
//////////////////////////////////////////////////

BOOLEAN
DirtyLoggingInitialize();

VOID DirtyLoggingHandleVmexits(VIRTUAL_MACHINE_STATE* VCpu);

VOID DirtyLoggingEnable();

VOID DirtyLoggingDisable();
