/**
 * @file pch.h
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 *
 * @details Pre-compiled headers
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//
// Exclude rarely-used stuff from Windows headers
//
#define WIN32_LEAN_AND_MEAN

//
// Windows Header Files
//
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "SDK/Imports/HyperDbgSymImports.h"
#include "common.h"
#include "scanner.h"
#include "globals.h"
#include "..\script-eval\header\ScriptEngineCommonDefinitions.h"
#include "script-engine.h"
#include "parse-table.h"
