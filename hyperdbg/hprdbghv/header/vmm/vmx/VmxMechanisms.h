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
//					Functions					//
//////////////////////////////////////////////////

VOID
VmxMechanismCreateImmediateVmexit();

VOID
VmxMechanismDisableImmediateVmexit();
