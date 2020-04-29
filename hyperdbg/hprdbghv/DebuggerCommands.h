/**
 * @file DebuggerCommands.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Commands for debugger
 * @details This file contains general commands that are implemented for debugger 
 * These command mostly start without "!" or "."
 * 
 * @version 0.1
 * @date 2020-04-10
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once
#include <ntddk.h>
#include "Logging.h"

NTSTATUS
DebuggerCommandReadMemory(PDEBUGGER_READ_MEMORY ReadMemRequest, PVOID UserBuffer, PSIZE_T ReturnSize);

NTSTATUS
DebuggerReadOrWriteMsr(PDEBUGGER_READ_AND_WRITE_ON_MSR ReadOrWriteMsrRequest, PSIZE_T ReturnSize);
