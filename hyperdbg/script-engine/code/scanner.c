/**
 * @file scanner.c
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 *
 * @details Script Engine Scanner
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
 * @param c
 * @param str
 * @return PTOKEN
 */
PTOKEN
GetToken(char * c, char * str)
{
    PTOKEN Token = NewUnknownToken();

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
                    AppendByte(Token, '\n');
                    continue;
                }
                if (*c == '\\')
                {
                    AppendByte(Token, '\\');
                    continue;
                }
                else if (*c == 't')
                {
                    AppendByte(Token, '\t');
                    continue;
                }
                else if (*c == 'x')
                {
                    char ByteString[] = "000";
                    int  len          = (int)strlen(ByteString);
                    int  i            = 0;
                    for (; i < len; i++)
                    {
                        *c = sgetc(str);
                        if (!IsHex(*c))
                            break;

                        RotateLeftStringOnce(ByteString);
                        ByteString[len - 1] = *c;
                    }

                    if (i == 0 || i == 3)
                    {
                        Token->Type = UNKNOWN;
                        *c          = sgetc(str);
                        return Token;
                    }
                    else
                    {
                        InputIdx--;
                        char num = (char)strtol(ByteString, NULL, 16);
                        AppendByte(Token, num);
                    }
                }
                else if (*c == '"')
                {
                    AppendByte(Token, '"');
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
                AppendByte(Token, *c);
            }
        } while (1);

        Token->Len++;
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
                AppendByte(Token, *c);
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
            //
            // Append valid characters for pseudo registers' name
            //
            while (IsLetter(*c) || IsDecimal(*c) || *c == '_')
            {
                AppendByte(Token, *c);
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
        AppendByte(Token, *c);
        *c = sgetc(str);
        if (IsLetter(*c) || IsHex(*c) || (*c == '_') || (*c == '!'))
        {
            do
            {
                AppendByte(Token, *c);
                *c = sgetc(str);
            } while (IsLetter(*c) || IsHex(*c) || (*c == '_') || (*c == '!'));

            BOOLEAN WasFound = FALSE;
            BOOLEAN HasBang  = strstr(Token->Value, "!") != 0;
            UINT64  Address  = 0;

            if (HasBang)
            {
                Address = ScriptEngineConvertNameToAddress(Token->Value, &WasFound);
            }

            if (WasFound)
            {
                RemoveToken(&Token);
                char str[20] = {0};
                sprintf(str, "%llx", Address);
                Token = NewToken(HEX, str);
            }
            else
            {
                if (HasBang)
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
                    AppendByte(Token, *c);
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
                    AppendByte(Token, *c);
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
                    AppendByte(Token, *c);
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
                    AppendByte(Token, *c);
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
                    AppendByte(Token, *c);
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

    case 'L':
        if (*(str + InputIdx) == '"')
        {
            InputIdx++;
            do
            {
                *c = sgetc(str);

                if (*c == '\\')
                {
                    *c = sgetc(str);
                    if (*c == 'n')
                    {
                        AppendWchar(Token, L'\n');
                        continue;
                    }
                    if (*c == '\\')
                    {
                        AppendWchar(Token, L'\\');
                        continue;
                    }
                    else if (*c == 't')
                    {
                        AppendWchar(Token, L'\t');
                        continue;
                    }
                    else if (*c == 'x')
                    {
                        char ByteString[] = "00000";
                        int  len          = (int)strlen(ByteString);
                        int  i            = 0;
                        for (; i < len; i++)
                        {
                            *c = sgetc(str);
                            if (!IsHex(*c))
                                break;

                            RotateLeftStringOnce(ByteString);
                            ByteString[len - 1] = *c;
                        }

                        if (i == 0 || i == 5)
                        {
                            Token->Type = UNKNOWN;
                            *c          = sgetc(str);
                            return Token;
                        }
                        else
                        {
                            InputIdx--;
                            wchar_t num = (wchar_t)strtol(ByteString, NULL, 16);
                            AppendWchar(Token, num);
                        }
                    }
                    else if (*c == '"')
                    {
                        AppendWchar(Token, L'"');
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
                    AppendWchar(Token, (wchar_t)*c);
                }
            } while (1);

            Token->Len += 2;
            Token->Type = WSTRING;
            *c          = sgetc(str);
            return Token;
        }
        else

        default:
            if (*c >= '0' && *c <= '9')
            {
                do
                {
                    if (*c != '`')
                        AppendByte(Token, *c);
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
                        AppendByte(Token, *c);

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
                            AppendByte(Token, *c);
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
                        BOOLEAN HasBang  = strstr(Token->Value, "!") != 0;
                        UINT64  Address  = 0;

                        if (HasBang)
                        {
                            Address = ScriptEngineConvertNameToAddress(Token->Value, &WasFound);
                        }

                        if (WasFound)
                        {
                            RemoveToken(&Token);
                            char str[20] = {0};
                            sprintf(str, "%llx", Address);
                            Token = NewToken(HEX, str);
                        }
                        else
                        {
                            if (HasBang)
                            {
                                Token->Type = UNKNOWN;
                                return Token;
                            }
                            else
                            {
                                if (GetFunctionParameterIdentifier(Token) != -1)
                                {
                                    Token->Type = FUNCTION_PARAMETER_ID;
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
                        BOOLEAN HasBang  = strstr(Token->Value, "!") != 0;
                        UINT64  Address  = 0;

                        if (HasBang)
                        {
                            Address = ScriptEngineConvertNameToAddress(Token->Value, &WasFound);
                        }

                        if (WasFound)
                        {
                            RemoveToken(&Token);
                            char str[20] = {0};
                            sprintf(str, "%llx", Address);
                            Token = NewToken(HEX, str);
                        }
                        else
                        {
                            if (HasBang)
                            {
                                Token->Type = UNKNOWN;
                                return Token;
                            }
                            else
                            {
                                if (GetFunctionParameterIdentifier(Token) != -1)
                                {
                                    Token->Type = FUNCTION_PARAMETER_ID;
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
                        AppendByte(Token, *c);
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
                    BOOLEAN HasBang  = strstr(Token->Value, "!") != 0;
                    UINT64  Address  = 0;

                    if (HasBang)
                    {
                        Address = ScriptEngineConvertNameToAddress(Token->Value, &WasFound);
                    }

                    if (WasFound)
                    {
                        RemoveToken(&Token);
                        char str[20] = {0};
                        sprintf(str, "%llx", Address);
                        Token = NewToken(HEX, str);
                    }
                    else
                    {
                        if (HasBang)
                        {
                            Token->Type = UNKNOWN;
                            return Token;
                        }
                        else
                        {
                            if (GetFunctionParameterIdentifier(Token) != -1)
                            {
                                Token->Type = FUNCTION_PARAMETER_ID;
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
 * @brief Perform scanning the script engine
 *
 * @param str
 * @param c
 * @return PTOKEN
 */
PTOKEN
Scan(char * str, char * c)
{
    static BOOLEAN ReturnEndOfString;
    PTOKEN         Token;

    if (InputIdx <= 1)
    {
        ReturnEndOfString = FALSE;
    }

    if (ReturnEndOfString)
    {
        Token = NewToken(END_OF_STACK, "$");
        return Token;
    }

    if (str[InputIdx - 1] == '\0')
    {
    }
    while (1)
    {
        CurrentTokenIdx = InputIdx - 1;

        Token = GetToken(c, str);

        if ((char)*c == EOF)
        {
            ReturnEndOfString = TRUE;
        }

        if (Token->Type == WHITE_SPACE)
        {
            if (!strcmp(Token->Value, "\n"))
            {
                CurrentLine++;
                CurrentLineIdx = InputIdx;
            }
            RemoveToken(&Token);
            if (ReturnEndOfString)
            {
                Token = NewToken(END_OF_STACK, "$");
                return Token;
            }
            continue;
        }
        else if (Token->Type == COMMENT)
        {
            RemoveToken(&Token);
            if (ReturnEndOfString)
            {
                Token = NewToken(END_OF_STACK, "$");
                return Token;
            }
            continue;
        }
        return Token;
    }
}

/**
 * @brief returns last character of string
 *
 * @param str
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

/**
 * @brief Check whether a string is a keyword or not
 *
 * @param str
 * @return char
 */
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

/**
 * @brief Check if string is register or not
 *
 * @param str
 * @return char
 */
char
IsRegister(char * str)
{
    if (RegisterToInt(str) == INVALID)
        return 0;
    return 1;
}

/**
 * @brief eck if string is Id or not
 *
 * @param str
 * @return char
 */
char
IsId(char * str)
{
    // TODO: Check the str is a id or not
    return 0;
}
