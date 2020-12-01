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
#include "common.h"
#include "parse_table.h"




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
	TOKEN Token;

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
			RemoveToken(Token);
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
	int n = KEYWORD_LIST_LENGTH;
	for (int i = 0; i < n; i++)
	{if (!strcmp(str, KeywordList[i]))
		{
			return 1; 
		}
	}
	return 0;
}