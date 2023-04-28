/**
 * @file rev-ctrl.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief headers for controller of the reversing machine's module
 * @details
 * @version 0.2
 * @date 2023-03-23
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//            	    Functions                   //
//////////////////////////////////////////////////

BOOLEAN
RevRequestService(REVERSING_MACHINE_RECONSTRUCT_MEMORY_REQUEST * RevRequest);
