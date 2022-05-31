/**
 * @file CrossApi.h
 * @author Behrooz Abbassi (BehroozAbbassi@hyperdbg.org)
 * @brief Cross platform APIs
 * @details 
 * @version 0.1
 * @date 2022-01-17
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//				     Functions		      		//
//////////////////////////////////////////////////

PVOID
CrsAllocateContiguousZeroedMemory(_In_ SIZE_T NumberOfBytes);

PVOID
CrsAllocateNonPagedPool(SIZE_T NumberOfBytes);
