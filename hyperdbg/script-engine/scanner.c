/**
 * @file scanner.c
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 * @brief Script Engine Scanner
 * @details
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief reads a token from the input string
 *
 * @param string
 * @param refrence to last read character
 * @return Token
 */

TOKEN
GetToken(char * c, char * str)
{
    TOKEN Token = NewToken();

    switch (*c)
    {
    case '"':
        do
        {
            *c = sgetc(str);

            if (*c == '\\')
            {
                *c = sgetc(str);
                if (*c == 'n')
                {
                    Append(Token, '\n');
                    continue;
                }
                if (*c == '\\')
                {
                    Append(Token, '\\');
                    continue;
                }
                else if (*c == 't')
                {
                    Append(Token, '\t');
                    continue;
                }
                else if (*c == '"')
                {
                    Append(Token, '"');
                    continue;
                }
                else
                {
                    Token->Type = UNKNOWN;
                    *c          = sgetc(str);
                    return Token;
                }
            }
            else if (*c == '"')
            {
                break;
            }
            else
            {
                Append(Token, *c);
            }
        } while (1);

        Token->Type = STRING;
        *c          = sgetc(str);
        return Token;
    case '~':
        strcpy(Token->Value, "~");
        Token->Type = SPECIAL_TOKEN;
        *c          = sgetc(str);
        return Token;

    case '+':
        *c = sgetc(str);
        if (*c == '+')
        {
            strcpy(Token->Value, "++");
            Token->Type = SPECIAL_TOKEN;
            *c          = sgetc(str);
            return Token;
        }
        else if (*c == '=')
        {
            strcpy(Token->Value, "+=");
            Token->Type = SPECIAL_TOKEN;
            *c          = sgetc(str);
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
            *c          = sgetc(str);
            return Token;
        }
        else if (*c == '=')
        {
            strcpy(Token->Value, "-=");
            Token->Type = SPECIAL_TOKEN;
            *c          = sgetc(str);
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
            *c          = sgetc(str);
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
            *c          = sgetc(str);
            return Token;
        }
        else if (*c == '=')
        {
            strcpy(Token->Value, ">=");
            Token->Type = SPECIAL_TOKEN;
            *c          = sgetc(str);
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
            *c          = sgetc(str);
            return Token;
        }
        else if (*c == '=')
        {
            strcpy(Token->Value, "<=");
            Token->Type = SPECIAL_TOKEN;
            *c          = sgetc(str);
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
            *c          = sgetc(str);

            return Token;
        }
        else if (*c == '/')
        {
            do
            {
                *c = sgetc(str);
            } while (*c != '\n' && (int)*c != EOF);

            Token->Type = COMMENT;
            *c          = sgetc(str);
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
                        *c          = sgetc(str);
                        return Token;
                    }
                }
                if ((int)*c == EOF)
                    break;
            } while (1);

            Token->Type = UNKNOWN;
            *c          = sgetc(str);
            return Token;
        }
        else
        {
            strcpy(Token->Value, "/");
            Token->Type = SPECIAL_TOKEN;
            return Token;
        }

    case '=':
        *c = sgetc(str);
        if (*c == '=')
        {
            strcpy(Token->Value, "==");
            Token->Type = SPECIAL_TOKEN;
            *c          = sgetc(str);
            return Token;
        }
        else
        {
            strcpy(Token->Value, "=");
            Token->Type = SPECIAL_TOKEN;
            return Token;
        }
    case '!':
        *c = sgetc(str);
        if (*c == '=')
        {
            strcpy(Token->Value, "!=");
            Token->Type = SPECIAL_TOKEN;
            *c          = sgetc(str);
            return Token;
        }
        else
        {
            strcpy(Token->Value, "!");
            Token->Type = UNKNOWN;
            return Token;
        }
    case '%':
        strcpy(Token->Value, "%");
        Token->Type = SPECIAL_TOKEN;
        *c          = sgetc(str);
        return Token;

    case ',':
        strcpy(Token->Value, ",");
        Token->Type = SPECIAL_TOKEN;
        *c          = sgetc(str);
        return Token;

    case ';':
        strcpy(Token->Value, ";");
        Token->Type = SPECIAL_TOKEN;
        *c          = sgetc(str);
        return Token;

    case ':':
        strcpy(Token->Value, ":");
        Token->Type = SPECIAL_TOKEN;
        *c          = sgetc(str);
        return Token;

    case '(':
        strcpy(Token->Value, "(");
        Token->Type = SPECIAL_TOKEN;
        *c          = sgetc(str);
        return Token;
    case ')':
        strcpy(Token->Value, ")");
        Token->Type = SPECIAL_TOKEN;
        *c          = sgetc(str);
        return Token;
    case '{':
        strcpy(Token->Value, "{");
        Token->Type = SPECIAL_TOKEN;
        *c          = sgetc(str);
        return Token;
    case '}':
        strcpy(Token->Value, "}");
        Token->Type = SPECIAL_TOKEN;
        *c          = sgetc(str);
        return Token;
    case '|':
        *c = sgetc(str);
        if (*c == '|')
        {
            strcpy(Token->Value, "||");
            Token->Type = SPECIAL_TOKEN;
            *c          = sgetc(str);
            return Token;
        }
        else
        {
            strcpy(Token->Value, "|");
            Token->Type = SPECIAL_TOKEN;
            return Token;
        }
    case '&':
        *c = sgetc(str);
        if (*c == '&')
        {
            strcpy(Token->Value, "&&");
            Token->Type = SPECIAL_TOKEN;
            *c          = sgetc(str);
            return Token;
        }
        else
        {
            strcpy(Token->Value, "&");
            Token->Type = SPECIAL_TOKEN;
            return Token;
        }

    case '^':
        strcpy(Token->Value, "^");
        Token->Type = SPECIAL_TOKEN;
        *c          = sgetc(str);
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
            if (RegisterToInt(Token->Value) != INVALID)
            {
                Token->Type = REGISTER;
            }
            else
            {
                Token->Type = UNKNOWN;
            }
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
            if (PseudoRegToInt(Token->Value) != INVALID)
            {
                Token->Type = PSEUDO_REGISTER;
            }
            else
            {
                Token->Type = UNKNOWN;
            }

            return Token;
        }

    case '.':
        Append(Token, *c);
        *c = sgetc(str);
        if (IsLetter(*c) || IsHex(*c) || (*c == '_') || (*c == '!'))
        {
            do
            {
                Append(Token, *c);
                *c = sgetc(str);
            } while (IsLetter(*c) || IsHex(*c) || (*c == '_') || (*c == '!'));

            BOOLEAN WasFound = FALSE;
            UINT64  Address  = ScriptEngineConvertNameToAddress(Token->Value, &WasFound);
            if (WasFound)
            {
                free(Token->Value);
                char * str = malloc(20);
                sprintf(str, "%llx", Address);
                Token->Value = str;
                Token->Type  = HEX;
            }
            else
            {
                char BangChar[] = "!";
                if (strstr(Token->Value, BangChar))
                {
                    Token->Type = UNKNOWN;
                    return Token;
                }
                else
                {
                    if (GetGlobalIdentifierVal(Token) != -1)
                    {
                        Token->Type = GLOBAL_ID;
                    }
                    else
                    {
                        Token->Type = GLOBAL_UNRESOLVED_ID;
                    }
                }
            }
        }
        else
        {
            Token->Type = UNKNOWN;
            return Token;
        }
        return Token;

    case ' ':
    case '\t':
        strcpy(Token->Value, "");
        Token->Type = WHITE_SPACE;
        *c          = sgetc(str);
        return Token;
    case '\n':
        strcpy(Token->Value, "\n");
        Token->Type = WHITE_SPACE;
        *c          = sgetc(str);
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
        if (*c >= '0' && *c <= '9')
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
        else if ((*c >= 'a' && *c <= 'f') || (*c >= 'A' && *c <= 'F') || (*c == '_') || (*c == '!'))
        {
            uint8_t NotHex = 0;
            do
            {
                if (*c != '`')
                    Append(Token, *c);

                *c = sgetc(str);
                if (IsHex(*c) || *c == '`')
                {
                    // Nothing
                }
                else if ((*c >= 'G' && *c <= 'Z') || (*c >= 'g' && *c <= 'z'))
                {
                    NotHex = 1;
                    break;
                }
                else
                {
                    break;
                }
            } while (1);
            if (NotHex)
            {
                do
                {
                    if (*c != '`')
                        Append(Token, *c);
                    *c = sgetc(str);
                } while (IsLetter(*c) || IsHex(*c) || (*c == '_') || (*c == '!'));
                if (IsKeyword(Token->Value))
                {
                    Token->Type = KEYWORD;
                }
                else if (IsRegister(Token->Value))
                {
                    Token->Type = REGISTER;
                }
                else
                {
                    BOOLEAN WasFound = FALSE;
                    UINT64  Address  = ScriptEngineConvertNameToAddress(Token->Value, &WasFound);
                    if (WasFound)
                    {
                        free(Token->Value);
                        char * str = malloc(20);
                        sprintf(str, "%llx", Address);
                        Token->Value = str;
                        Token->Type  = HEX;
                    }
                    else
                    {
                        char BangChar[] = "!";
                        if (strstr(Token->Value, BangChar))
                        {
                            Token->Type = UNKNOWN;
                            return Token;
                        }
                        else
                        {
                            if (GetLocalIdentifierVal(Token) != -1)
                            {
                                Token->Type = LOCAL_ID;
                            }
                            else
                            {
                                Token->Type = LOCAL_UNRESOLVED_ID;
                            }
                        }
                    }
                }
                return Token;
            }
            else
            {
                if (IsKeyword(Token->Value))
                {
                    Token->Type = KEYWORD;
                }
                else if (IsRegister(Token->Value))
                {
                    Token->Type = REGISTER;
                }
                else if (IsId(Token->Value))
                {
                    BOOLEAN WasFound = FALSE;
                    UINT64  Address  = ScriptEngineConvertNameToAddress(Token->Value, &WasFound);
                    if (WasFound)
                    {
                        free(Token->Value);
                        char * str = malloc(20);
                        sprintf(str, "%llx", Address);
                        Token->Value = str;
                        Token->Type  = HEX;
                    }
                    else
                    {
                        if (strstr(Token->Value, "!"))
                        {
                            Token->Type = UNKNOWN;
                            return Token;
                        }
                        else
                        {
                            if (GetLocalIdentifierVal(Token) != -1)
                            {
                                Token->Type = LOCAL_ID;
                            }
                            else
                            {
                                Token->Type = LOCAL_UNRESOLVED_ID;
                            }
                        }
                    }
                }
                else
                {
                    Token->Type = HEX;
                }
                return Token;
            }
        }
        else if ((*c >= 'G' && *c <= 'Z') || (*c >= 'g' && *c <= 'z') || (*c == '_') || (*c == '!'))
        {
            do
            {
                if (*c != '`')
                    Append(Token, *c);
                *c = sgetc(str);
            } while (IsLetter(*c) || IsHex(*c) || (*c == '_') || (*c == '!'));
            if (IsKeyword(Token->Value))
            {
                Token->Type = KEYWORD;
            }
            else if (IsRegister(Token->Value))
            {
                Token->Type = REGISTER;
            }
            else
            {
                BOOLEAN WasFound = FALSE;
                UINT64  Address  = ScriptEngineConvertNameToAddress(Token->Value, &WasFound);
                if (WasFound)
                {
                    free(Token->Value);
                    char * str = malloc(20);
                    sprintf(str, "%llx", Address);
                    Token->Value = str;
                    Token->Type  = HEX;
                }
                else
                {
                    char BangChar[] = "!";
                    if (strstr(Token->Value, BangChar))
                    {
                        Token->Type = UNKNOWN;
                        return Token;
                    }
                    else
                    {
                        if (GetLocalIdentifierVal(Token) != -1)
                        {
                            Token->Type = LOCAL_ID;
                        }
                        else
                        {
                            Token->Type = LOCAL_UNRESOLVED_ID;
                        }
                    }
                }
            }
            return Token;
        }

        Token->Type = UNKNOWN;
        *c          = sgetc(str);
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
TOKEN
Scan(char * str, char * c)
{
    TOKEN Token;

    while (1)
    {
        CurrentTokenIdx = InputIdx - 1;

        Token = GetToken(c, str);

        //
        // check end of string
        //
        if ((int)*c == EOF)
        {
            Token->Type = END_OF_STACK;
            strcpy(Token->Value, "$");
            return Token;
        }
        else if (Token->Type == WHITE_SPACE)
        {
            if (!strcmp(Token->Value, "\n"))
            {
                CurrentLine++;
                CurrentLineIdx = InputIdx;
            }
            RemoveToken(Token);
            continue;
        }
        else if (Token->Type == COMMENT)
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
char
sgetc(char * str)
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

char
IsKeyword(char * str)
{
    int n = KEYWORD_LIST_LENGTH;
    for (int i = 0; i < n; i++)
    {
        if (!strcmp(str, KeywordList[i]))
        {
            return 1;
        }
    }
    n = TERMINAL_COUNT;
    for (int i = 0; i < n; i++)
    {
        if (!strcmp(str, TerminalMap[i]))
        {
            return 1;
        }
    }

    return 0;
}

char
IsRegister(char * str)
{
    if (RegisterToInt(str) == INVALID)
        return 0;
    return 1;
}
char
IsId(char * str)
{
    // TODO: Check the str is a id or not
    return 0;
}
