#pragma once

//
// Global Variables
//

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
