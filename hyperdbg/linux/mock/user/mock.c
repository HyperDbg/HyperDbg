/**
 * @file mock.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Mock user-mode application for testing the HyperDbg
 * @details
 * @version 0.19
 * @date 2026-04-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

int
main(void)
{
    printf("Hello world HyperDbg!\n");

    //////////////////////////////////////////////////////////////
    //
    // Test for working intrinsics
    //
    CpuReadTsc();
    CpuCpuId(NULL, 0);

    //////////////////////////////////////////////////////////////

    return 0;
}
