/**
 * @file GlobalVariableManagement.h
 * @author Behrooz Abbassi (BehroozAbbassi@hyperdbg.org)
 * @brief Headers for management of global variables
 * @details
 * @version 0.1
 * @date 2022-03-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

BOOLEAN
GlobalGuestStateAllocateZeroedMemory(VOID);

VOID
    GlobalGuestStateFreeMemory(VOID);
