/**
 * @file ScriptEngine.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers for script engine functions used in kernel-mode evaluator
 * of the script engine
 * @details
 * @version 0.2
 * @date 2022-06-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

UINT64
ScriptEngineWrapperGetInstructionPointer();

UINT64
ScriptEngineWrapperGetAddressOfReservedBuffer(PDEBUGGER_EVENT_ACTION Action);
