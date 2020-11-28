/**
 * @file scanner.c
 * @author M.H. Gholamrezei (gholamrezaei.mh@gmail.com)
 * @brief Script Engine Scanner
 * @details
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "scanner.h"


// TODO: automate generation of KeyWordList

/**
* @brief list of keywords
*/
const char* KeywordList[] =
{
	"str",
	"low",
	"poi",
	"db",
	"dq",
	"dw",
	"not",
	"dd",
	"func",
	"wstr",
	"sizeof",
	"neg",
	"hi",
	"print"
};

/**
 * @brief allocates a new token 
 *
 * @return Token
 */
TOKEN NewToken()
{
	TOKEN Token;

	//
	// Allocates memory for token and its value
	//
	Token = (TOKEN)malloc(sizeof(*Token));
	Token->Value = (char*)calloc(TOKEN_VALUE_MAX_LEN, sizeof(char));

	//
	// Init fields 
	//
	strcpy(Token->Value, "");
	Token->Type = UNKNOWN;
	Token->len = 0;
	Token->max_len = TOKEN_VALUE_MAX_LEN;

	return Token;
}

/**
 * @brief removes allocated memory of a token
 *
 * @param Token
 */
void RemoveToken(TOKEN Token)
{
	free(Token->Value);
	free(Token);

	return;
}

/**
 * @brief prints token 
 * @detail prints value and type of token 
 * 
 * @param Token
 */
void PrintToken(TOKEN Token)
{

	// 
	// Prints vlaue of the Token 
	// 
	if (Token->Type == WHITE_SPACE)
	{
		printf("< :");
	}
	else
	{
		printf("<'%s' : ", Token->Value);
	}

	//
	// Prints type of the Token
	//
	switch (Token->Type)
	{
	case ID:
		printf(" ID>\n");
		break;
	case DECIMAL:
		printf(" DECIMAL>\n");
		break;
	case HEX:
		printf(" HEX>\n");
		break;
	case OCTAL:
		printf(" OCTAL>\n");
		break;
	case BINARY:
		printf(" BINARY>\n");
		break;
	case SPECIAL_TOKEN:
		printf(" SPECIAL_TOKEN>\n");
		break;
	case KEYWORD:
		printf(" KEYWORD>\n");
		break;
	case WHITE_SPACE:
		printf(" WHITE_SPACE>\n");
		break;
	case COMMENT:
		printf(" COMMENT>\n");
		break;
	case REGISTER:
		printf(" REGISTER>\n");
		break;
	case PSEUDO_REGISTER:
		printf(" PSEUDO_REGISTER>\n");
		break;
	case SEMANTIC_RULE:
		printf(" SEMANTIC_RULE>\n");
		break;
	case NON_TERMINAL:
		printf(" NON_TERMINAL>\n");
		break;
	case END_OF_STACK:
		printf(" END_OF_STACK>\n");
		break;
	case UNKNOWN:
		printf(" UNKNOWN>\n");
		break;

	default:
		printf(" ERROR>\n");
		break;
	}
}

/**
 * @brief appends char to the token value 
 *
 * @param Token 
 * @param char 
 */
void Append(TOKEN Token, char c)
{
	//
	// Check overflow of the string
	//
	if (Token->len >= Token->max_len-1)
	{
		//
		// Double the length of the allocated space for the string
		//
		Token->max_len *= 2; 
		char* NewValue = (char*)calloc(Token->max_len, sizeof(char));


		// 
		// Free Old buffer and update the pointer 
		//
		strncpy(NewValue, Token->Value, Token->len);
		free(Token->Value);
		Token->Value = NewValue;
	}


	// 
	// Append the new charcter to the string 
	//
	strncat(Token->Value, &c, 1);
	Token->len++;


	
}

/**
 * @brief allocates a new TOKEN_LIST 
 *
 * @return TOKEN_LIST
 */
TOKEN_LIST NewTokenList(void)
{
	TOKEN_LIST TokenList;

	//
	// Allocation of memory for TOKEN_LIST structure 
	//
	TokenList = (TOKEN_LIST)malloc(sizeof(*TokenList));
	
	
	//
	// Initialize fields of TOKEN_LIST
	//
	TokenList->Pointer = 0;
	TokenList->Size = TOKEN_LIST_INIT_SIZE;

	//
	// Allocation of memory for TOKEN_LIST buffer
	//
	TokenList->Head = (TOKEN*)malloc(TokenList->Size * sizeof(TOKEN));
	
	return TokenList;
}

/**
 * @brief removes allocated memory of a TOKEN_LIST
 *
 * @param TokenList
 */
void RemoveTokenList(TOKEN_LIST TokenList)
{
	free(TokenList->Head);
	free(TokenList);

	return;
}


/**
 * @brief prints each Token inside a TokenList
 *
 * @param TokenList
 */
void PrintTokenList(TOKEN_LIST TokenList)
{
	TOKEN Token;
	for (uintptr_t i = 0; i < TokenList->Pointer; i++)
	{
		Token = *(TokenList->Head + i);
		PrintToken(Token);
	}

}

/**
 * @brief adds Token to the last empty position of TokenList
 *
 * @param Token
 * @param TokenList
 * @return TokenList
 */
TOKEN_LIST Push(TOKEN_LIST TokenList, TOKEN Token)
{
	//
	// Calculate address to write new token
	//
	uintptr_t Head = (uintptr_t)TokenList->Head;
	uintptr_t Pointer = (uintptr_t)TokenList->Pointer;
	TOKEN* WriteAddr = (TOKEN*)(Head + Pointer * sizeof(TOKEN));

	//
	// Write Token to appropriate address in TokenList
	//
	*WriteAddr = Token;

	//
	// Update Pointer
	//
	TokenList->Pointer++;

	//
	// Handle overflow
	//
	if (Pointer == TokenList->Size - 1)
	{
		//
		// Allocate a new buffer for string list with doubled length
		//
		TOKEN* NewHead = (TOKEN*)malloc(2 * TokenList->Size * sizeof(TOKEN));

		//
		// Copy old buffer to new buffer
		//
		memcpy(NewHead, TokenList->Head, TokenList->Size * sizeof(TOKEN));

		//
		// Free old buffer
		//
		free(TokenList->Head);

		//
		// Update Head and size of TokenList
		//
		TokenList->Size = TokenList->Size * 2;
		TokenList->Head = NewHead;
	}

	return TokenList;
}
/**
 * @brief removes last Token of a TokenList and returns it
 *
 * @param TokenList
 @ @return Token
 */
TOKEN Pop(TOKEN_LIST TokenList)
{
	//
	// Calculate address to read most recent token
	//
	if (TokenList->Pointer > 0)
		TokenList->Pointer--;
	uintptr_t Head = (uintptr_t)TokenList->Head;
	uintptr_t Pointer = (uintptr_t)TokenList->Pointer;
	TOKEN* ReadAddr = (TOKEN*)(Head + Pointer * sizeof(TOKEN));


	return *ReadAddr;
}


/**
 * @brief returns last Token of a TokenList 
 *
 * @param TokenList
 * @return Token
 */
TOKEN Top(TOKEN_LIST TokenList)
{
	//
	// Calculate address to read most recent token
	//
	uintptr_t Head = (uintptr_t)TokenList->Head;
	uintptr_t Pointer = (uintptr_t)TokenList->Pointer - 1;
	TOKEN* ReadAddr = (TOKEN*)(Head + Pointer * sizeof(TOKEN));

	return *ReadAddr;
}

/**
* @brief cheks whether input char belongs to hexadecimal digit-set or not
*
* @param char
* @return bool
*/
char IsHex(char c)
{
	if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
		return 1;
	else
		return 0;
}

/**
* @brief cheks whether input char belongs to decimal digit-set or not
*
* @param char
* @return bool
*/
char IsDecimal(char c)
{
	if (c >= '0' && c <= '9')
		return 1;
	else
		return 0;
}

/**
* @brief cheks whether input char belongs to alphabet set or not
*
* @param char
* @return bool
*/
char IsLetter(char c)
{
	if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
		return 1;
	else
	{
		return 0;
	}
}

/**
* @brief cheks whether input char belongs to binary digit-set or not
*
* @param char
* @return bool
*/
char IsBinary(char c)
{
	if (c == '0' || c == '1')
		return 1;
	else
	{
		return 0;
	}
}

/**
* @brief cheks whether input char belongs to octal digit-set or not
*
* @param char
* @return bool
*/
char IsOctal(char c)
{
	if (c >= '0' && c <= '7')
		return 1;
	else
		return 0;
}


/**
* @brief reads a token from the input string
*
* @param string 
* @param refrence to last read character 
* @return Token
*/

TOKEN GetToken(char* c, char* str)
{
	 
	TOKEN Token = NewToken();

	switch (*c)
	{
	case '+':
		*c = sgetc(str);
		if (*c == '+')
		{
			strcpy(Token->Value, "++");
			Token->Type = SPECIAL_TOKEN;
			*c = sgetc(str);
			return Token;
		}
		else if (*c == '=')
		{
			strcpy(Token->Value, "+=");
			Token->Type = SPECIAL_TOKEN;
			*c = sgetc(str);
			return Token;
		}
		else
		{
			strcpy(Token->Value, "+");
			Token->Type = SPECIAL_TOKEN;
			return Token;
		}
	case '-':
		*c = sgetc(str);
		if (*c == '-')
		{
			strcpy(Token->Value, "--");
			Token->Type = SPECIAL_TOKEN;
			*c = sgetc(str);
			return Token;
		}
		else if (*c == '=')
		{
			strcpy(Token->Value, "-=");
			Token->Type = SPECIAL_TOKEN;
			*c = sgetc(str);
			return Token;
		}
		else
		{
			strcpy(Token->Value, "-");
			Token->Type = SPECIAL_TOKEN;
			return Token;
		}
	case '*':
		*c = sgetc(str);
		if (*c == '=')
		{
			strcpy(Token->Value, "*=");
			Token->Type = SPECIAL_TOKEN;
			*c = sgetc(str);
			return Token;
		}
		else
		{
			strcpy(Token->Value, "*");
			Token->Type = SPECIAL_TOKEN;
			return Token;
		}
	case '>':
		*c = sgetc(str);
		if (*c == '>')
		{
			strcpy(Token->Value, ">>");
			Token->Type = SPECIAL_TOKEN;
			*c = sgetc(str);
			return Token;
		}
		else
		{
			strcpy(Token->Value, ">");
			Token->Type = SPECIAL_TOKEN;
			return Token;
		}
	case '<':
		*c = sgetc(str);
		if (*c == '<')
		{
			strcpy(Token->Value, "<<");
			Token->Type = SPECIAL_TOKEN;
			*c = sgetc(str);
			return Token;
		}
		else
		{
			strcpy(Token->Value, "<");
			Token->Type = SPECIAL_TOKEN;
			return Token;
		}
	case '/':
		*c = sgetc(str);
		if (*c == '=')
		{
			strcpy(Token->Value, "/=");
			Token->Type = SPECIAL_TOKEN;
			*c = sgetc(str);

			return Token;
		}
		else if (*c == '/')
		{
			do
			{
				*c = sgetc(str);
			} while (*c != '\n' && *c != EOF);

			Token->Type = COMMENT;
			*c = sgetc(str);
			return Token;
		}
		else if (*c == '*')
		{
			do
			{
				*c = sgetc(str);
				if (*c == '*')
				{
					*c = sgetc(str);
					if (*c == '/')
					{
						Token->Type = COMMENT;
						*c = sgetc(str);
						return Token;
					}
				}
				if (*c == EOF)
					break;
			} while (1);

			Token->Type = UNKNOWN;
			*c = sgetc(str);
			return Token;
		}
		else
		{
			strcpy(Token->Value, "/");
			Token->Type = SPECIAL_TOKEN;
			return Token;
		}

	case '=':
		strcpy(Token->Value, "=");
		Token->Type = SPECIAL_TOKEN;
		*c = sgetc(str);
		return Token;
	case '%':
		strcpy(Token->Value, "%");
		Token->Type = SPECIAL_TOKEN;
		*c = sgetc(str);
		return Token;

	case ',':
		strcpy(Token->Value, ",");
		Token->Type = SPECIAL_TOKEN;
		*c = sgetc(str);
		return Token;

	case ';':
		strcpy(Token->Value, ";");
		Token->Type = SPECIAL_TOKEN;
		*c = sgetc(str);
		return Token;

	case ':':
		strcpy(Token->Value, ":");
		Token->Type = SPECIAL_TOKEN;
		*c = sgetc(str);
		return Token;

	case '(':
		strcpy(Token->Value, "(");
		Token->Type = SPECIAL_TOKEN;
		*c = sgetc(str);
		return Token;
	case ')':
		strcpy(Token->Value, ")");
		Token->Type = SPECIAL_TOKEN;
		*c = sgetc(str);
		return Token;
	case '{':
		strcpy(Token->Value, "{");
		Token->Type = SPECIAL_TOKEN;
		*c = sgetc(str);
		return Token;
	case '}':
		strcpy(Token->Value, "}");
		Token->Type = SPECIAL_TOKEN;
		*c = sgetc(str);
		return Token;
	case '|':
		strcpy(Token->Value, "|");
		Token->Type = SPECIAL_TOKEN;
		*c = sgetc(str);
		return Token;
	case '&':
		strcpy(Token->Value, "&");
		Token->Type = SPECIAL_TOKEN;
		*c = sgetc(str);
		return Token;
	case '^':
		strcpy(Token->Value, "^");
		Token->Type = SPECIAL_TOKEN;
		*c = sgetc(str);
		return Token;
	case '@':
		*c = sgetc(str);
		if (IsLetter(*c))
		{
			while (IsLetter(*c) || IsDecimal(*c))
			{
				Append(Token, *c);
				*c = sgetc(str);
			}
			Token->Type = REGISTER;
			return Token;
		}

	case '$':
		*c = sgetc(str);
		if (IsLetter(*c))
		{
			while (IsLetter(*c) || IsDecimal(*c))
			{
				Append(Token, *c);
				*c = sgetc(str);
			}
			Token->Type = PSEUDO_REGISTER;
			return Token;
		}

	case '.':
		*c = sgetc(str);
		if (IsHex(*c))
		{
		}
		else
		{
		}

	case ' ':
	case '\t':
		strcpy(Token->Value, "");
		Token->Type = WHITE_SPACE;
		*c = sgetc(str);
		return Token;
	case '\n':
		strcpy(Token->Value, "");
		Token->Type = WHITE_SPACE;
		*c = sgetc(str);
		return Token;

	case '0':
		*c = sgetc(str);
		if (*c == 'x')
		{
			*c = sgetc(str);
			while (IsHex(*c) || *c == '`')
			{
				if (*c != '`')
					Append(Token, *c);
				*c = sgetc(str);
			}
			Token->Type = HEX;
			return Token;
		}
		else if (*c == 'o')
		{
			*c = sgetc(str);
			while (IsOctal(*c) || *c == '`')
			{
				if (*c != '`')
					Append(Token, *c);
				*c = sgetc(str);
			}
			Token->Type = OCTAL;
			return Token;
		}
		else if (*c == 'n')
		{
			*c = sgetc(str);
			while (IsDecimal(*c) || *c == '`')
			{
				if (*c != '`')
					Append(Token, *c);
				*c = sgetc(str);
			}
			Token->Type = DECIMAL;
			return Token;
		}
		else if (*c == 'y')
		{
			*c = sgetc(str);
			while (IsBinary(*c) || *c == '`')
			{
				if (*c != '`')
					Append(Token, *c);
				*c = sgetc(str);
			}
			Token->Type = BINARY;
			return Token;
		}

		else if (IsHex(*c))
		{
			do
			{
				if (*c != '`')
					Append(Token, *c);
				*c = sgetc(str);
			} while (IsHex(*c) || *c == '`');
			Token->Type = HEX;
			return Token;
		}
		else
		{
			strcpy(Token->Value, "0");
			Token->Type = HEX;
			return Token;
		}

	default:
		if (IsHex(*c))
		{
			do
			{
				if (*c != '`')
					Append(Token, *c);
				*c = sgetc(str);
			} while (IsHex(*c) || *c == '`');
			if (IsKeyword(Token->Value))
			{
				Token->Type = KEYWORD;
			}
			else
			{
				Token->Type = HEX;
			}
			
			return Token;
		}
		else if (IsLetter(*c) || *c == '_')
		{
			while (IsLetter(*c) || *c == '_' || IsDecimal(*c))
			{
				Append(Token, *c);
				*c = sgetc(str);
			}

			if (IsKeyword(Token->Value))
			{
				Token->Type = KEYWORD;
			}
			else
			{
				Token->Type = ID;
			}

			return Token;
		}


		Token->Type = UNKNOWN;
		*c = sgetc(str);
		return Token;
	}
	return Token;
}
/**
* @brief reads a token and returns if it is not white space or comment
*
* @param string 
* @param refrence to last read character
*/
TOKEN Scan(char* str, char* c)
{
	TOKEN Token = NewToken();

	while (1)
	{

		Token = GetToken(c, str);
		
		//
		// check end of string 
		//
		if (*c == EOF)
		{
			Token->Type = END_OF_STACK;
			strcpy(Token->Value, "$");
			return Token;
		}

		else if (Token->Type == WHITE_SPACE || Token->Type == COMMENT)
		{
			continue;
		}
		return Token;
	}
}

/**
* @brief returns last character of string 
* 
* @pram string 
* @return last character 
*/
char sgetc(char* str)
{
	char c = str[InputIdx];
	if (c)
	{
		InputIdx++;
		return c;
	}
	else
	{
		return EOF;
	}
}

char IsKeyword(char* str)
{
	int n = sizeof(KeywordList) / sizeof(KeywordList[0]);
	for (int i = 0; i < n; i++)
	{
		if (!strcmp(str, KeywordList[i]))
		{
			return 1; 
		}
	}
	return 0;
}