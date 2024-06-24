/**
 * @file VmxMechanisms.h
 * @author Sina Karvandi (sina@hyperdbg.org)
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

#define IMMEDIATE_VMEXIT_MECHANISM_VECTOR_FOR_SELF_IPI IPI_INTERRUPT

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

VOID
VmxMechanismCreateImmediateVmexit(VIRTUAL_MACHINE_STATE * VCpu);

VOID
VmxMechanismHandleImmediateVmexit(VIRTUAL_MACHINE_STATE * VCpu);
