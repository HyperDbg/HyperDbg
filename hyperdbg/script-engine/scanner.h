#pragma once

#ifndef SCANNER_H
#define SCANNER_H 
#include "common.h"
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
* @brief curren token start postion
*/
unsigned int CurrentTokenIdx;


////////////////////////////////////////////////////
// Interfacing functions						  // 
////////////////////////////////////////////////////
TOKEN GetToken(char* c, char* str);

TOKEN Scan(char* str, char* c);

char sgetc(char* str);

char IsKeyword(char* str);

#endif // !SCANNER_H


