/**
 * @file globals.c
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 *
 * @details Global variables
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

PSCRIPT_ENGINE_TOKEN_LIST   GlobalIdTable;
PUSER_DEFINED_FUNCTION_NODE UserDefinedFunctionHead;
PUSER_DEFINED_FUNCTION_NODE CurrentUserDefinedFunction;
PINCLUDE_NODE               IncludeHead;
unsigned int                InputIdx;
unsigned int                CurrentLine;
unsigned int                CurrentLineIdx;
unsigned int                CurrentTokenIdx;
HWDBG_INSTANCE_INFORMATION  g_HwdbgInstanceInfo;
BOOLEAN                     g_HwdbgInstanceInfoIsValid;
PVOID                       g_MessageHandler;
