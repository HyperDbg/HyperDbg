/**
 * @file BreakpointCommands.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Commands for setting breakpoints
 * 
 * @version 0.1
 * @date 2021-03-12
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

BOOLEAN
BreakpointAddNew(PDEBUGGEE_BP_PACKET BpDescriptorArg);

VOID
BreakpointListOrModify(PDEBUGGEE_BP_LIST_OR_MODIFY_PACKET ListOrModifyBreakpoints);
