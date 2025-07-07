/**
 * @file scanner.h
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 *
 * @details Scanner headers
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#ifndef SCANNER_H
#    define SCANNER_H

/**
 * @brief lookup table for storing global Ids
 */
PSCRIPT_ENGINE_TOKEN_LIST GlobalIdTable;

/**
 * @brief
 */
PUSER_DEFINED_FUNCTION_NODE UserDefinedFunctionHead;

PUSER_DEFINED_FUNCTION_NODE CurrentUserDefinedFunction;

/**
 * @brief number of read characters from input
 */
unsigned int InputIdx;

/**
 * @brief number of current reading line
 */
unsigned int CurrentLine;

/*
 * @brief current line start position
 */
unsigned int CurrentLineIdx;

/*
 * @brief current PTOKEN start position
 */
unsigned int CurrentTokenIdx;

////////////////////////////////////////////////////
//            Interfacing functions	         	  //
////////////////////////////////////////////////////

PSCRIPT_ENGINE_TOKEN
GetToken(char * c, char * str);

PSCRIPT_ENGINE_TOKEN
Scan(char * str, char * c);

char
sgetc(char * str);

char
IsKeyword(char * str);

char
IsId(char * str);

char
IsRegister(char * str);

char
IsVariableType(char * str);
#endif // !SCANNER_H
