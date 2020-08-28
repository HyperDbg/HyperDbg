/**
 * @file CrossVmexits.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief The functions for passing vm-exits in vmx root 
 * @details
 * @version 0.1
 * @date 2020-06-14
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief Handling XSETBV Instruction vm-exits
 * 
 * @param Reg 
 * @param Value 
 * @return VOID 
 */
VOID
VmxHandleXsetbv(UINT32 Reg, UINT64 Value)
{
    _xsetbv(Reg, Value);
}
