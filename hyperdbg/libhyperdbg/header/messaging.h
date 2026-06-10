/**
 * @file messaging.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief headers for messaging functions
 * @details
 * @version 0.19
 * @date 2026-05-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				    Functions                   //
//////////////////////////////////////////////////

VOID
SetTextMessageCallback(PVOID Handler);

PVOID
SetTextMessageCallbackUsingSharedBuffer(PVOID Handler);

VOID
UnsetTextMessageCallback();
