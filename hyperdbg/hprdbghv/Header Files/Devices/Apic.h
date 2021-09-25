/**
 * @file Apic.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Headers relating to Advanced Programmable Interrupt Controller (APIC)
 * @details
 * @version 0.1
 * @date 2020-12-31
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				   Definition					//
//////////////////////////////////////////////////

#define X2_MSR_BASE 0x800
#define ICROffset   0x300
#define TO_X2(x)    (x / 0x10)

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

VOID
ApicTriggerGenericNmi(UINT32 CurrentCoreIndex);

BOOLEAN
ApicInitialize();

VOID
ApicUninitialize();
