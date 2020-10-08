/**
 * @file Evaluation.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Interpret and evaluate the expressions from the scripting engine (header)
 * @details
 * @version 0.1
 * @date 2020-10-08
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

NTSTATUS
EvaluationInterpretPrintRequest(PDEBUGGER_PRINT DebuggerPrintRequest, PVOID BufferToSendToUsermode, PUINT SizeOfBufferToSendToUsermode);
