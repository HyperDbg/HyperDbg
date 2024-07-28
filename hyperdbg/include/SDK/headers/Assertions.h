/**
 * @file Assertions.h
 * @author ddkwork
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg's assertions
 * @details This file contains asserts and static asserts
 * @version 0.10
 * @date 2024-06-21
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//                  Asserts                     //
//////////////////////////////////////////////////

/**
 * @brief check so the DEBUGGEE_UD_PAUSED_PACKET should be smaller than packet size
 *
 */
static_assert(sizeof(DEBUGGEE_UD_PAUSED_PACKET) < PacketChunkSize,
              "err (static_assert), size of PacketChunkSize should be bigger than DEBUGGEE_UD_PAUSED_PACKET");

/**
 * @brief check so the DEBUGGER_UPDATE_SYMBOL_TABLE should be smaller than packet size
 *
 */
static_assert(sizeof(DEBUGGER_UPDATE_SYMBOL_TABLE) < PacketChunkSize,
              "err (static_assert), size of PacketChunkSize should be bigger than DEBUGGER_UPDATE_SYMBOL_TABLE (MODULE_SYMBOL_DETAIL)");
