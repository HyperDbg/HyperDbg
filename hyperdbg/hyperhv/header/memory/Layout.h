/**
 * @file Layout.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header files for working with memory layouts
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

CR3_TYPE
LayoutGetCr3ByProcessId(_In_ UINT32 ProcessId);

UINT64
LayoutGetSystemDirectoryTableBase();
