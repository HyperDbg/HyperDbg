/**
 * @file SerialConnection.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Serial port connection from debuggee to debugger
 * @details
 * @version 0.1
 * @date 2020-12-15
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

VOID
SerialConnectionTest()
{
    for (size_t i = 0; i < 100; i++)
    {
        KdHyperDbgTest((UINT16)i);
    }
}
