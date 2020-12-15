/**
 * @file SerialConnection.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Header for serial port connection from debuggee to debugger
 * @details
 * @version 0.1
 * @date 2020-12-15
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//			  External Functions				//
//////////////////////////////////////////////////

UINT64
KdHyperDbgTest(UINT16 Byte);

//////////////////////////////////////////////////
//					 Functions					//
//////////////////////////////////////////////////

VOID
SerialConnectionTest();
