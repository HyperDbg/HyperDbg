/**
 * @file HyperDbgHyperEvade.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers relating exported functions from hyperevade (transparency) module
 * @version 0.14
 * @date 2025-06-07
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#ifdef HYPERDBG_HYPEREVADE
#    define IMPORT_EXPORT_HYPEREVADE __declspec(dllexport)
#else
#    define IMPORT_EXPORT_HYPEREVADE __declspec(dllimport)
#endif

//////////////////////////////////////////////////
//            hyperevade functions 	    		//
//////////////////////////////////////////////////

IMPORT_EXPORT_HYPEREVADE BOOLEAN
TransparentHideDebugger(HYPEREVADE_CALLBACKS *                        HyperevadeCallbacks,
                        DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE * TransparentModeRequest);

IMPORT_EXPORT_HYPEREVADE BOOLEAN
TransparentUnhideDebugger(DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE * TransparentModeRequest);

IMPORT_EXPORT_HYPEREVADE VOID
TransparentCheckAndModifyCpuid(INT32 CpuInfo[], PGUEST_REGS Regs);

IMPORT_EXPORT_HYPEREVADE VOID
TransparentHandleSystemCallHook(GUEST_REGS * Regs);

IMPORT_EXPORT_HYPEREVADE VOID
TransparentCallbackHandleAfterSyscall(GUEST_REGS *                      Regs,
                                      UINT32                            ProcessId,
                                      UINT32                            ThreadId,
                                      UINT64                            Context,
                                      SYSCALL_CALLBACK_CONTEXT_PARAMS * Params);
