
#pragma once

//
// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.
//

#ifndef PCH_H
#    define PCH_H

//
// Exclude rarely-used stuff from Windows headers
//
#    define WIN32_LEAN_AND_MEAN

//
// Windows Header Files
//
#    include <windows.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>
#    include <stdint.h>

#    include "common.h"
#    include "scanner.h"
#    include "globals.h"
#    include "ScriptEngineCommonDefinitions.h"
#    include "script-engine.h"
#    include "parse-table.h"

#endif //PCH_H
