/**
 * @file VmxMechanisms.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief VMX based mechanisms header
 * @details
 * @version 0.1
 * @date 2021-12-16
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//					Constants					//
//////////////////////////////////////////////////

#define IMMEDIATE_VMEXIT_MECHANISM_VECTOR_FOR_SELF_IPI 0x55

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

VOID
VmxMechanismCreateImmediateVmexit(UINT32 CurrentCoreIndex);

VOID
VmxMechanismHandleImmediateVmexit(UINT32 CurrentCoreIndex, PGUEST_REGS GuestRegs);
