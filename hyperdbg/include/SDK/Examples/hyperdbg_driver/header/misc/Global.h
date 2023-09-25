/**
 * @file Global.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers for global variables
 * @version 0.2
 * @date 2023-01-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

/**
 * @brief Determines whether the one application gets the handle or not
 * this is used to ensure that only one application can get the handle
 *
 */
BOOLEAN g_HandleInUse;

/**
 * @brief Determines whether the clients are allowed to send IOCTL to the drive or not
 *
 */
BOOLEAN g_AllowIOCTLFromUsermode;
