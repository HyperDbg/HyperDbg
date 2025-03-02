/**
 * @file MmioShadowing.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header files for MMIO shadowing
 * @details
 * @version 0.14
 * @date 2025-02-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

VOID
MmioShadowingApplyPageModification(VIRTUAL_MACHINE_STATE * VCpu,
                                   PEPT_HOOKED_PAGE_DETAIL HookedPage);
