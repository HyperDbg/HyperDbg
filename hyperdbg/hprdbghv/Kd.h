/**
 * @file Kd.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Header for routines related to kernel debugging
 * @details 
 * @version 0.1
 * @date 2020-12-20
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */

#pragma once

VOID
KdHaltSystem(PDEBUGGER_PAUSE_PACKET_RECEIVED PausePacket);

VOID
KdManageSystemHaltOnVmxRoot();
