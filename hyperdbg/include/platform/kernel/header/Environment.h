/**
 * @file Environment.h
 * @author Behrooz Abbassi (BehroozAbbassi@hyperdbg.org)
 * @brief The running environment of HyperDbg
 * @details
 * @version 0.1
 * @date 2022-01-17
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				    Definitions	        		//
//////////////////////////////////////////////////

//
// Check for platform
//

#if defined(_WIN32) || defined(_WIN64)
#    define ENV_WINDOWS
#elif defined(__linux__)
#    error "This code cannot compile on Linux yet"
#    define ENV_LINUX
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
#    error "This code cannot compile on BSD yet"
#    define ENV_BSD
#else
#    error "This code cannot compile on non-Windows, non-Linux, and non-BSD platforms"
#endif
