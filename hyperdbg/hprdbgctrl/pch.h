/**
 * @file pch.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief header file corresponding to the pre-compiled header
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

//
// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for
// future builds. This also affects IntelliSense performance, including code
// completion and many code browsing features. However, files listed here are
// ALL re-compiled if any one of them is updated between builds. Do not add
// files here that you will be updating frequently as this negates the
// performance advantage.
//

#ifndef PCH_H
#define PCH_H

//
// add headers that you want to pre-compile here
//

#include <Windows.h>
#include <algorithm>
#include <conio.h>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <strsafe.h>
#include <time.h>
#include <vector>
#include <winioctl.h>
#include <winternl.h>
#include <array>
#include <bitset>
#include <intrin.h>
#include <inttypes.h>

//
// HyperDbg defined headers
//
#include "Configuration.h"
#include "Definition.h"
#include "commands.h"
#include "common.h"
#include "debugger.h"
#include "framework.h"
#include "help.h"
#include "hprdbgctrl.h"
#include "list.h"

#endif // PCH_H

#pragma comment(lib,"ntdll.lib")
