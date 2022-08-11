/**
 * @file ScriptEngineHeader.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers for exported script engine headers
 * @details
 * @version 0.2
 * @date 2022-06-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

BOOL
ScriptEngineExecute(PGUEST_REGS                    GuestRegs,
                    ACTION_BUFFER *                ActionDetail,
                    SCRIPT_ENGINE_VARIABLES_LIST * VariablesList,
                    SYMBOL_BUFFER *                CodeBuffer,
                    int *                          Indx,
                    SYMBOL *                       ErrorOperator);

UINT64
GetRegValue(PGUEST_REGS GuestRegs, REGS_ENUM RegId);

VOID
ScriptEngineGetOperatorName(PSYMBOL OperatorSymbol, CHAR * BufferForName);

VOID
ScriptEngineGetOperatorName(PSYMBOL OperatorSymbol, CHAR * BufferForName);
