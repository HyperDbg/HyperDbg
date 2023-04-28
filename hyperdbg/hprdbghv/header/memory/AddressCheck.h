/**
 * @file AddressCheck.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header files for address checks
 * @details
 * @version 0.2
 * @date 2023-04-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

//
// Some functions are globally defined in SDK
//

BOOLEAN
CheckAddressCanonicality(UINT64 VAddr, PBOOLEAN IsKernelAddress);
