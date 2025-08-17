/**
 * @file Smm.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers relating to operations related to System Management Mode (SMM)
 * @details
 *
 * @version 0.15
 * @date 2025-08-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				   Constants					//
//////////////////////////////////////////////////

/**
 * @brief SMI trigger port value
 *
 */
#define SMI_TRIGGER_POWER_VALUE 0x80

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

BOOLEAN
SmmPerformSmiOperation(SMI_OPERATION_PACKETS * SmiOperationRequest, BOOLEAN ApplyFromVmxRootMode);
