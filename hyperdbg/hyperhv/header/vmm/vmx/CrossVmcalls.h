/**
 * @file CrossVmcalls.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers relating to cross (standalone) VMCALLs
 * @details
 * @version 0.19
 * @date 2026-04-14
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

UINT64
CrossVmcallGetDebugctlVmcallOnTargetCore();

VOID
CrossVmcallSetDebugctlVmcallOnTargetCore(UINT64 Value);
