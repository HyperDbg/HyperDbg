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
#pragma once

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
#    define PCH_H

//
// add headers that you want to pre-compile here
//

#    define WIN32_LEAN_AND_MEAN

#    include <winsock2.h>
#    include <ws2tcpip.h>
#    include <Windows.h>
#    include <algorithm>
#    include <array>
#    include <bitset>
#    include <conio.h>
#    include <intrin.h>
#    include <inttypes.h>
#    include <iomanip>
#    include <iostream>
#    include <iterator>
#    include <sstream>
#    include <stdio.h>
#    include <stdlib.h>
#    include <map>
#    include <string>
#    include <strsafe.h>
#    include <time.h>
#    include <vector>
#    include <winioctl.h>
#    include <winternl.h>
#    include <fstream>
#    include <shlobj.h>
#    include <tchar.h>
#    include <numeric>
#    include <tlhelp32.h>
#    include <VersionHelpers.h>

//
// HyperDbg defined headers
//
#    include "ScriptEngineCommonDefinitions.h"
#    include "Configuration.h"
#    include "Definition.h"
#    include "commands.h"
#    include "common.h"
#    include "debugger.h"
#    include "exports.h"
#    include "help.h"
#    include "install.h"
#    include "list.h"
#    include "tests.h"
#    include "transparency.h"
#    include "communication.h"
#    include "namedpipe.h"
#    include "forwarding.h"
#    include "kd.h"

#endif // PCH_H

#pragma comment(lib, "ntdll.lib")

//
// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
// for tcpclient.cpp and tcpserver.cpp
//
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")
