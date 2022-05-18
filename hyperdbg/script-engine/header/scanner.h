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
* @brief lookup table for storing Ids
*/
PTOKEN_LIST IdTable;

/**
* @brief number of read characters from input
*/
unsigned int InputIdx;

/**
* @brief number of current reading line
*/
unsigned int CurrentLine;

/*
* @brief current line start postion  
*/
unsigned int CurrentLineIdx;

/*
* @brief curren PTOKEN start postion
*/
unsigned int CurrentTokenIdx;

////////////////////////////////////////////////////
//            Interfacing functions	         	  //
////////////////////////////////////////////////////

PTOKEN
GetToken(char * c, char * str);

PTOKEN
Scan(char * str, char * c);

char
sgetc(char * str);

char
IsKeyword(char * str);

char
IsId(char * str);

char
IsRegister(char * str);
#endif // !SCANNER_H
