/**
 * @file platform_mem.h
 * @author Behrooz Abbassi (BehroozAbbassi@hyperdbg.org)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Cross platform APIs for memory allocation
 * @details
 * @version 0.1
 * @date 2022-01-17
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once
#include "platform_types.h"

//////////////////////////////////////////////////
//				     Functions		      		//
//////////////////////////////////////////////////

PLAT_STATUS
PlatformReadMemory(
    PLAT_PTR  Process,
    PLAT_PTR  Address,
    PLAT_PTR  Buffer,
    PLAT_SIZE Size
);

PLAT_STATUS
PlatformWriteMemory(
    PLAT_PTR  Process,
    PLAT_PTR  Address,
    PLAT_PTR  Buffer,
    PLAT_SIZE Size
);

PLAT_PTR
PlatformAllocMemory(
    PLAT_SIZE Size
);

void
PlatformFreeMemory(
    PLAT_PTR Memory
);
//#elif defined(_WIN32)
//#else
//#error "Unsupported platform"
//#endif
