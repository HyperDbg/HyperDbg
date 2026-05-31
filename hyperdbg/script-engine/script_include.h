/**
 * @file script_include.h
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 *
 * @brief Include file resolver declarations
 * @details
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#define MAX_PATH_LEN 1024

VOID
ResolveIncludePath(const char * IncludeFilePath, char * OutPath);

BOOLEAN
FileExists(const char * Path);

BOOLEAN
ParseIncludeFile(char * IncludeFile, char ** Buffer);

char *
InsertStrNew(char * Str, int InputIdx, const char * Buf);