/**
 * @file common.c
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 *
 * @details Common routines
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Allocates a new token
 *
 * @return Token
 */
PTOKEN
NewUnknownToken()
{
    PTOKEN Token;

    //
    // Allocate memory for token and its value
    //
    Token = (PTOKEN)malloc(sizeof(TOKEN));

    if (Token == NULL)
    {
        //
        // There was an error allocating buffer
        //
        return NULL;
    }

    Token->Value = (char *)calloc(TOKEN_VALUE_MAX_LEN + 1, sizeof(char));

    if (Token->Value == NULL)
    {
        //
        // There was an error allocating buffer
        //
        return NULL;
    }

    //
    // Init fields
    //
    strcpy(Token->Value, "");
    Token->Type   = UNKNOWN;
    Token->Len    = 0;
    Token->MaxLen = TOKEN_VALUE_MAX_LEN;

    return Token;
}

PTOKEN
NewToken(TOKEN_TYPE Type, char * Value)
{
    //
    // Allocate memory for token]
    //
    PTOKEN Token = (PTOKEN)malloc(sizeof(TOKEN));

    if (Token == NULL)
    {
        //
        // There was an error allocating buffer
        //
        return NULL;
    }

    //
    // Init fields
    //
    unsigned int Len = (unsigned int)strlen(Value);
    Token->Type      = Type;
    Token->Len       = Len;
    Token->MaxLen    = Len;
    Token->Value     = (char *)calloc(Token->MaxLen + 1, sizeof(char));

    if (Token->Value == NULL)
    {
        //
        // There was an error allocating buffer
        //
        return NULL;
    }

    strcpy(Token->Value, Value);

    return Token;
}

/**
 * @brief Removes allocated memory of a token
 *
 * @param Token
 */
void
RemoveToken(PTOKEN * Token)
{
    free((*Token)->Value);
    free(*Token);
    *Token = NULL;
    return;
}

/**
 * @brief Prints token
 * @detail prints value and type of token
 *
 * @param Token
 */
void
PrintToken(PTOKEN Token)
{
    //
    // Prints value of the Token
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
    case GLOBAL_ID:
        printf(" GLOBAL_ID>\n");
        break;
    case GLOBAL_UNRESOLVED_ID:
        printf(" GLOBAL_UNRESOLVED_ID>\n");
        break;
    case LOCAL_ID:
        printf(" LOCAL_ID>\n");
        break;
    case LOCAL_UNRESOLVED_ID:
        printf(" LOCAL_UNRESOLVED_ID>\n");
        break;
    case STATE_ID:
        printf(" STATE_ID>\n");
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
    case STRING:
        printf(" STRING>\n");
        break;
    case WSTRING:
        printf(" WSTRING>\n");
        break;
    case TEMP:
        printf(" TEMP>\n");
        break;
    case UNKNOWN:
        printf(" UNKNOWN>\n");
        break;
    case INPUT_VARIABLE_TYPE:
        printf(" INPUT_VARIABLE_TYPE>\n");
        break;
    case HANDLED_VARIABLE_TYPE:
        printf(" HANDLED_VARIABLE_TYPE>\n");
        break;
    case FUNCTION_TYPE:
        printf(" FUNCTION_TYPE>\n");
        break;
    case FUNCTION_PARAMETER_ID:
        printf(" FUNCTION_PARAMETER_ID>\n");
        break;
    case STACK_TEMP:
        printf(" STACK_TEMP>\n");
    default:
        printf(" ERROR>\n");
        break;
    }
}

/**
 * @brief Appends char to the token value
 *
 * @param Token
 * @param char
 */
void
AppendByte(PTOKEN Token, char c)
{
    //
    // Check overflow of the string
    //
    if (Token->Len >= Token->MaxLen - 1)
    {
        //
        // Double the length of the allocated space for the string
        //
        Token->MaxLen *= 2;
        char * NewValue = (char *)calloc(Token->MaxLen + 1, sizeof(char));

        if (NewValue == NULL)
        {
            printf("err, could not allocate buffer");
            return;
        }

        //
        // Free Old buffer and update the pointer
        //
        memcpy(NewValue, Token->Value, Token->Len);
        free(Token->Value);
        Token->Value = NewValue;
    }

    //
    // Append the new character to the string
    //
    Token->Value[Token->Len] = c;
    Token->Len++;
}

/**
 * @brief Appends wchar_t to the token value
 *
 * @param Token
 * @param char
 */
void
AppendWchar(PTOKEN Token, wchar_t c)
{
    //
    // Check overflow of the string
    //
    if (Token->Len >= Token->MaxLen - 2)
    {
        //
        // Double the length of the allocated space for the wstring
        //
        Token->MaxLen *= 2;
        char * NewValue = (char *)calloc(Token->MaxLen + 2, sizeof(char));

        if (NewValue == NULL)
        {
            printf("err, could not allocate buffer");
            return;
        }

        //
        // Free Old buffer and update the pointer
        //
        memcpy(NewValue, Token->Value, Token->Len);
        free(Token->Value);
        Token->Value = NewValue;
    }

    //
    // Append the new character to the wstring
    //
    *((wchar_t *)(Token->Value) + Token->Len / 2) = c;
    Token->Len += 2;
}

/**
 * @brief Copies a PTOKEN
 *
 * @return PTOKEN
 */
PTOKEN
CopyToken(PTOKEN Token)
{
    PTOKEN TokenCopy = (PTOKEN)malloc(sizeof(TOKEN));

    if (TokenCopy == NULL)
    {
        //
        // There was an error allocating buffer
        //
        return NULL;
    }

    TokenCopy->Type   = Token->Type;
    TokenCopy->MaxLen = Token->MaxLen;
    TokenCopy->Len    = Token->Len;
    TokenCopy->Value  = (char *)calloc(strlen(Token->Value) + 1, sizeof(char));

    if (TokenCopy->Value == NULL)
    {
        //
        // There was an error allocating buffer
        //
        return NULL;
    }

    strcpy(TokenCopy->Value, Token->Value);

    return TokenCopy;
}

/**
 * allocates a new TOKEN_LIST
 *
 * @return TOKEN_LIST
 */
PTOKEN_LIST
NewTokenList(void)
{
    PTOKEN_LIST TokenList = NULL;

    //
    // Allocation of memory for TOKEN_LIST structure
    //
    TokenList = (PTOKEN_LIST)malloc(sizeof(*TokenList));

    if (TokenList == NULL)
    {
        //
        // There was an error allocating buffer
        //
        return NULL;
    }

    //
    // Initialize fields of TOKEN_LIST
    //
    TokenList->Pointer = 0;
    TokenList->Size    = TOKEN_LIST_INIT_SIZE;

    //
    // Allocation of memory for TOKEN_LIST buffer
    //
    TokenList->Head = (PTOKEN *)malloc(TokenList->Size * sizeof(PTOKEN));

    return TokenList;
}

/**
 * @brief Removes allocated memory of a TOKEN_LIST
 *
 * @param TokenList
 */
void
RemoveTokenList(PTOKEN_LIST TokenList)
{
    PTOKEN Token;
    for (uintptr_t i = 0; i < TokenList->Pointer; i++)
    {
        Token = *(TokenList->Head + i);
        RemoveToken(&Token);
    }
    free(TokenList->Head);
    free(TokenList);

    return;
}

/**
 * @brief Prints each Token inside a TokenList
 *
 * @param TokenList
 */
void
PrintTokenList(PTOKEN_LIST TokenList)
{
    PTOKEN Token;
    for (uintptr_t i = 0; i < TokenList->Pointer; i++)
    {
        Token = *(TokenList->Head + i);
        PrintToken(Token);
    }
}

/**
 * @brief Adds Token to the last empty position of TokenList
 *
 * @param Token
 * @param TokenList
 * @return TokenList
 */
PTOKEN_LIST
Push(PTOKEN_LIST TokenList, PTOKEN Token)
{
    //
    // Calculate address to write new token
    //
    uintptr_t Head      = (uintptr_t)TokenList->Head;
    uintptr_t Pointer   = (uintptr_t)TokenList->Pointer;
    PTOKEN *  WriteAddr = (PTOKEN *)(Head + Pointer * sizeof(PTOKEN));

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
        PTOKEN * NewHead = (PTOKEN *)malloc(2 * TokenList->Size * sizeof(PTOKEN));

        if (NewHead == NULL)
        {
            printf("err, could not allocate buffer");
            return NULL;
        }

        //
        // Copy old buffer to new buffer
        //
        memcpy(NewHead, TokenList->Head, TokenList->Size * sizeof(PTOKEN));

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
 * @brief Removes last Token of a TokenList and returns it
 *
 * @param TokenList
 @ @return Token
 */
PTOKEN
Pop(PTOKEN_LIST TokenList)
{
    //
    // Calculate address to read most recent token
    //
    if (TokenList->Pointer > 0)
        TokenList->Pointer--; // not consider what if the token's type is string or wstring
    uintptr_t Head     = (uintptr_t)TokenList->Head;
    uintptr_t Pointer  = (uintptr_t)TokenList->Pointer;
    PTOKEN *  ReadAddr = (PTOKEN *)(Head + Pointer * sizeof(PTOKEN));

    return *ReadAddr;
}

/**
 * @brief Returns last Token of a TokenList
 *
 * @param TokenList
 * @return Token
 */
PTOKEN
Top(PTOKEN_LIST TokenList)
{
    //
    // Calculate address to read most recent pushed token
    //
    uintptr_t Head     = (uintptr_t)TokenList->Head;
    uintptr_t Pointer  = (uintptr_t)TokenList->Pointer - 1;
    PTOKEN *  ReadAddr = (PTOKEN *)(Head + Pointer * sizeof(PTOKEN));

    return *ReadAddr;
}

/**
 * @brief Checks whether input char belongs to hexadecimal digit-set or not
 *
 * @param char
 * @return bool
 */
char
IsHex(char c)
{
    if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
        return 1;
    else
        return 0;
}

/**
 * @brief Checks whether input char belongs to decimal digit-set or not
 *
 * @param char
 * @return bool
 */
char
IsDecimal(char c)
{
    if (c >= '0' && c <= '9')
        return 1;
    else
        return 0;
}

/**
 * @brief Checks whether input char belongs to alphabet set or not
 *
 * @param char
 * @return bool
 */
char
IsLetter(char c)
{
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
        return 1;
    else
    {
        return 0;
    }
}

/**
 * @brief Checks whether input char belongs to binary digit-set or not
 *
 * @param char
 * @return bool
 */
char
IsBinary(char c)
{
    if (c == '0' || c == '1')
        return 1;
    else
    {
        return 0;
    }
}

/**
 * @brief Checks whether input char belongs to octal digit-set or not
 *
 * @param char
 * @return bool
 */
char
IsOctal(char c)
{
    if (c >= '0' && c <= '7')
        return 1;
    else
        return 0;
}

/**
 * @brief Allocates a new temporary variable and returns it
 *
 * @param Error
 * @return PTOKEN
 */
PTOKEN
NewTemp(PSCRIPT_ENGINE_ERROR_TYPE Error, PSYMBOL CurrentFunctionSymbol)
{
    if (CurrentFunctionSymbol)
    {
        return NewStackTemp(Error);
    }
    else
    {
        static unsigned int TempID = 0;
        int                 i;
        for (i = 0; i < MAX_TEMP_COUNT; i++)
        {
            if (TempMap[i] == 0)
            {
                TempID     = i;
                TempMap[i] = 1;
                break;
            }
        }
        if (i == MAX_TEMP_COUNT)
        {
            *Error = SCRIPT_ENGINE_ERROR_TEMP_LIST_FULL;
        }
        PTOKEN Temp = NewUnknownToken();
        char   TempValue[8];
        sprintf(TempValue, "%d", TempID);
        strcpy(Temp->Value, TempValue);
        Temp->Type = TEMP;
        return Temp;
    }
}

/**
 * @brief Allocates a new temporary variable in stack and returns it
 *
 * @param Error
 * @return PTOKEN
 */
PTOKEN
NewStackTemp(PSCRIPT_ENGINE_ERROR_TYPE Error)
{
    static unsigned int TempID = 0;
    int                 i;
    for (i = 0; i < MAX_TEMP_COUNT; i++)
    {
        if (StackTempMap[i] == 0)
        {
            TempID          = i;
            StackTempMap[i] = 1;
            break;
        }
    }
    if (i == MAX_TEMP_COUNT)
    {
        *Error = SCRIPT_ENGINE_ERROR_TEMP_LIST_FULL;
    }
    PTOKEN Temp = NewUnknownToken();
    char   TempValue[8];
    sprintf(TempValue, "%d", TempID);
    strcpy(Temp->Value, TempValue);
    Temp->Type = STACK_TEMP;
    return Temp;
}

/**
 * @brief Frees the memory allocated by Temp
 *
 * @param Temp
 */
void
FreeTemp(PTOKEN Temp)
{
    int id = (int)DecimalToInt(Temp->Value);
    if (Temp->Type == TEMP)
    {
        TempMap[id] = 0;
    }
    else if (Temp->Type == STACK_TEMP)
    {
        StackTempMap[id] = 0;
    }
}

/**
 * @brief Resets the temporary variables map
 *
 */
void
CleanTempList(void)
{
    for (int i = 0; i < MAX_TEMP_COUNT; i++)
    {
        TempMap[i]      = 0;
        StackTempMap[i] = 0;
    }
}

/**
 * @brief Checks whether this Token type is OneOpFunc1
 *
 * @param Operator
 * @return char
 */
char
IsType1Func(PTOKEN Operator)
{
    unsigned int n = ONEOPFUNC1_LENGTH;
    for (unsigned int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, OneOpFunc1[i]))
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Checks whether this Token type is OneOpFunc2
 *
 * @param Operator
 * @return char
 */
char
IsType2Func(PTOKEN Operator)
{
    unsigned int n = ONEOPFUNC2_LENGTH;
    for (unsigned int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, OneOpFunc2[i]))
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Checks whether this Token type is OperatorsTwoOperandList
 *
 * @param Operator
 * @return char
 */
char
IsTwoOperandOperator(PTOKEN Operator)
{
    unsigned int n = OPERATORS_TWO_OPERAND_LIST_LENGTH;
    for (unsigned int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, OperatorsTwoOperandList[i]))
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Checks whether this Token type is OperatorsOneOperandList
 *
 * @param Operator
 * @return char
 */
char
IsOneOperandOperator(PTOKEN Operator)
{
    unsigned int n = OPERATORS_ONE_OPERAND_LIST_LENGTH;
    for (unsigned int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, OperatorsOneOperandList[i]))
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Checks whether this Token type is VarArgFunc1
 *
 * @param Operator
 * @return char
 */
char
IsType4Func(PTOKEN Operator)
{
    unsigned int n = VARARGFUNC1_LENGTH;
    for (unsigned int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, VarArgFunc1[i]))
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Checks whether this Token type is ZeroOpFunc1
 *
 * @param Operator
 * @return char
 */
char
IsType5Func(PTOKEN Operator)
{
    unsigned int n = ZEROOPFUNC1_LENGTH;
    for (unsigned int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, ZeroOpFunc1[i]))
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Checks whether this Token type is TwoOpFunc1
 *
 * @param Operator
 * @return char
 */
char
IsType6Func(PTOKEN Operator)
{
    unsigned int n = TWOOPFUNC1_LENGTH;
    for (unsigned int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, TwoOpFunc1[i]))
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Checks whether this Token type is TwoOpFunc2
 *
 * @param Operator
 * @return char
 */
char
IsType7Func(PTOKEN Operator)
{
    unsigned int n = TWOOPFUNC2_LENGTH;
    for (unsigned int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, TwoOpFunc2[i]))
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Checks whether this Token type is ThreeOpFunc1
 *
 * @param Operator
 * @return char
 */
char
IsType8Func(PTOKEN Operator)
{
    unsigned int n = THREEOPFUNC1_LENGTH;
    for (unsigned int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, ThreeOpFunc1[i]))
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Checks whether this Token type is OneOpFunc3
 *
 * @param Operator
 * @return char
 */
char
IsType9Func(PTOKEN Operator)
{
    unsigned int n = ONEOPFUNC3_LENGTH;
    for (unsigned int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, OneOpFunc3[i]))
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Checks whether this Token type is TwoOpFunc3
 *
 * @param Operator
 * @return char
 */
char
IsType10Func(PTOKEN Operator)
{
    unsigned int n = TWOOPFUNC3_LENGTH;
    for (unsigned int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, TwoOpFunc3[i]))
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Checks whether this Token type is ThreeOpFunc3
 *
 * @param Operator
 * @return char
 */
char
IsType11Func(PTOKEN Operator)
{
    unsigned int n = THREEOPFUNC3_LENGTH;
    for (unsigned int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, ThreeOpFunc3[i]))
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Checks whether this Token type is OneOpFunc4
 *
 * @param Operator
 * @return char
 */
char
IsType12Func(PTOKEN Operator)
{
    unsigned int n = ONEOPFUNC4_LENGTH;
    for (unsigned int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, OneOpFunc4[i]))
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Checks whether this Token type is TwoOpFunc4
 *
 * @param Operator
 * @return char
 */
char
IsType13Func(PTOKEN Operator)
{
    unsigned int n = TWOOPFUNC4_LENGTH;
    for (unsigned int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, TwoOpFunc4[i]))
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Checks whether this Token type is ThreeOpFunc2
 *
 * @param Operator
 * @return char
 */
char
IsType14Func(PTOKEN Operator)
{
    unsigned int n = THREEOPFUNC2_LENGTH;
    for (unsigned int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, ThreeOpFunc2[i]))
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Checks whether this Token type is VariableType
 *
 * @param Operator
 * @return char
 */

char
IsVariableType(PTOKEN Operator)
{
    unsigned int n = VARIABLETYPE_LENGTH;
    for (unsigned int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, VARIABLETYPE[i]))
        {
            return 1;
        }
    }

    return 0;
}

/**
 * @brief Checks whether this Token is noneterminal
 * NoneTerminal token starts with capital letter
 *
 * @param Token
 * @return char
 */
char
IsNoneTerminal(PTOKEN Token)
{
    if (Token->Value[0] >= 'A' && Token->Value[0] <= 'Z')
        return 1;
    else
        return 0;
}

/**
 * @brief Checks whether this Token is semantic rule
 * SemanticRule token starts with '@'
 *
 * @param Token
 * @return char
 */
char
IsSemanticRule(PTOKEN Token)
{
    if (Token->Value[0] == '@')
        return 1;
    else
        return 0;
}

/**
 * @brief Gets the Non Terminal Id object
 *
 * @param Token
 * @return int
 */
int
GetNonTerminalId(PTOKEN Token)
{
    for (int i = 0; i < NONETERMINAL_COUNT; i++)
    {
        if (!strcmp(Token->Value, NoneTerminalMap[i]))
            return i;
    }
    return INVALID;
}

/**
 * @brief Gets the Terminal Id object
 *
 * @param Token
 * @return int
 */
int
GetTerminalId(PTOKEN Token)
{
    for (int i = 0; i < TERMINAL_COUNT; i++)
    {
        if (Token->Type == HEX)
        {
            if (!strcmp("_hex", TerminalMap[i]))
                return i;
        }
        else if (Token->Type == GLOBAL_ID || Token->Type == GLOBAL_UNRESOLVED_ID)
        {
            if (!strcmp("_global_id", TerminalMap[i]))
            {
                return i;
            }
        }
        else if (Token->Type == LOCAL_ID || Token->Type == LOCAL_UNRESOLVED_ID)
        {
            if (!strcmp("_local_id", TerminalMap[i]))
            {
                return i;
            }
        }
        else if (Token->Type == FUNCTION_PARAMETER_ID)
        {
            if (!strcmp("_function_parameter_id", TerminalMap[i]))
            {
                return i;
            }
        }
        else if (Token->Type == REGISTER)
        {
            if (!strcmp("_register", TerminalMap[i]))
            {
                return i;
            }
        }
        else if (Token->Type == PSEUDO_REGISTER)
        {
            if (!strcmp("_pseudo_register", TerminalMap[i]))
            {
                return i;
            }
        }
        else if (Token->Type == DECIMAL)
        {
            if (!strcmp("_decimal", TerminalMap[i]))
            {
                return i;
            }
        }
        else if (Token->Type == BINARY)
        {
            if (!strcmp("_binary", TerminalMap[i]))
            {
                return i;
            }
        }
        else if (Token->Type == OCTAL)
        {
            if (!strcmp("_octal", TerminalMap[i]))
            {
                return i;
            }
        }
        else if (Token->Type == STRING)
        {
            if (!strcmp("_string", TerminalMap[i]))
            {
                return i;
            }
        }
        else if (Token->Type == WSTRING)
        {
            if (!strcmp("_wstring", TerminalMap[i]))
            {
                return i;
            }
        }
        else // Keyword
        {
            if (!strcmp(Token->Value, TerminalMap[i]))
                return i;
        }
    }
    return INVALID;
}

/**
 * @brief Gets the Non Terminal Id object
 *
 * @param Token
 * @return int
 */
int
LalrGetNonTerminalId(PTOKEN Token)
{
    for (int i = 0; i < LALR_NONTERMINAL_COUNT; i++)
    {
        if (!strcmp(Token->Value, LalrNoneTerminalMap[i]))
            return i;
    }
    return INVALID;
}

/**
 * @brief Gets the Terminal Id object
 *
 * @param Token
 * @return int
 */
int
LalrGetTerminalId(PTOKEN Token)
{
    for (int i = 0; i < LALR_TERMINAL_COUNT; i++)
    {
        if (Token->Type == HEX)
        {
            if (!strcmp("_hex", LalrTerminalMap[i]))
                return i;
        }
        else if (Token->Type == GLOBAL_ID || Token->Type == GLOBAL_UNRESOLVED_ID)
        {
            if (!strcmp("_global_id", LalrTerminalMap[i]))
            {
                return i;
            }
        }
        else if (Token->Type == LOCAL_ID || Token->Type == LOCAL_UNRESOLVED_ID)
        {
            if (!strcmp("_local_id", LalrTerminalMap[i]))
            {
                return i;
            }
        }
        else if (Token->Type == FUNCTION_PARAMETER_ID)
        {
            if (!strcmp("_function_parameter_id", LalrTerminalMap[i]))
            {
                return i;
            }
        }
        else if (Token->Type == REGISTER)
        {
            if (!strcmp("_register", LalrTerminalMap[i]))
            {
                return i;
            }
        }
        else if (Token->Type == PSEUDO_REGISTER)
        {
            if (!strcmp("_pseudo_register", LalrTerminalMap[i]))
            {
                return i;
            }
        }
        else if (Token->Type == DECIMAL)
        {
            if (!strcmp("_decimal", LalrTerminalMap[i]))
            {
                return i;
            }
        }
        else if (Token->Type == BINARY)
        {
            if (!strcmp("_binary", LalrTerminalMap[i]))
            {
                return i;
            }
        }
        else if (Token->Type == OCTAL)
        {
            if (!strcmp("_octal", LalrTerminalMap[i]))
            {
                return i;
            }
        }
        else if (Token->Type == STRING)
        {
            if (!strcmp("_string", LalrTerminalMap[i]))
            {
                return i;
            }
        }
        else if (Token->Type == WSTRING)
        {
            if (!strcmp("_wstring", LalrTerminalMap[i]))
            {
                return i;
            }
        }
        else // Keyword
        {
            if (!strcmp(Token->Value, LalrTerminalMap[i]))
                return i;
        }
    }
    return INVALID;
}

/**
 * @brief Checks whether the value and type of Token1 and Token2 are the same
 *
 * @param Token1
 * @param Token2
 * @return char
 */
char
IsEqual(const PTOKEN Token1, const PTOKEN Token2)
{
    if (Token1->Type == Token2->Type)
    {
        if (Token1->Type == SPECIAL_TOKEN)
        {
            if (!strcmp(Token1->Value, Token2->Value))
            {
                return 1;
            }
        }
        else
        {
            return 1;
        }
    }
    if (Token1->Type == GLOBAL_ID && Token2->Type == GLOBAL_UNRESOLVED_ID)
    {
        return 1;
    }
    if (Token1->Type == GLOBAL_UNRESOLVED_ID && Token2->Type == GLOBAL_ID)
    {
        return 1;
    }

    if (Token1->Type == LOCAL_ID && Token2->Type == LOCAL_UNRESOLVED_ID)
    {
        return 1;
    }
    if (Token1->Type == LOCAL_UNRESOLVED_ID && Token2->Type == LOCAL_ID)
    {
        return 1;
    }

    return 0;
}

/**
 * @brief Set the Type object
 *
 * @param Val
 * @param Type
 */
void
SetType(unsigned long long * Val, unsigned char Type)
{
    *Val = (unsigned long long int)Type;
}

/**
 * @brief Converts an decimal string to a integer
 *
 * @param str
 * @return unsigned long long int
 */
unsigned long long
DecimalToInt(char * str)
{
    unsigned long long Acc = 0;
    size_t             Len;

    Len = strlen(str);
    for (int i = 0; i < Len; i++)
    {
        Acc *= 10;
        Acc += (str[i] - '0');
    }
    return Acc;
}

/**
 * @brief Converts an decimal string to a signed integer
 *
 * @param str
 * @return unsigned long long
 */
unsigned long long
DecimalToSignedInt(char * str)
{
    long long Acc = 0;
    size_t    Len;

    if (str[0] == '-')
    {
        Len = strlen(str);
        for (int i = 1; i < Len; i++)
        {
            Acc *= 10;
            Acc += (str[i] - '0');
        }
        return -Acc;
    }
    else
    {
        Len = strlen(str);
        for (int i = 0; i < Len; i++)
        {
            Acc *= 10;
            Acc += (str[i] - '0');
        }
        return Acc;
    }
}

/**
 * @brief Converts an hexadecimal string to integer
 *
 * @param str
 * @return unsigned long long
 */
unsigned long long
HexToInt(char * str)
{
    char               temp;
    size_t             len = strlen(str);
    unsigned long long Acc = 0;
    for (int i = 0; i < len; i++)
    {
        Acc <<= 4;
        if (str[i] >= '0' && str[i] <= '9')
        {
            temp = str[i] - '0';
        }
        else if (str[i] >= 'a' && str[i] <= 'f')
        {
            temp = str[i] - 'a' + 10;
        }
        else
        {
            temp = str[i] - 'A' + 10;
        }
        Acc += temp;
    }

    return Acc;
}

/**
 * @brief Converts an octal string to integer
 *
 * @param str
 * @return unsigned long long
 */
unsigned long long
OctalToInt(char * str)
{
    size_t             Len;
    unsigned long long Acc = 0;

    Len = strlen(str);

    for (int i = 0; i < Len; i++)
    {
        Acc <<= 3;
        Acc += (str[i] - '0');
    }
    return Acc;
}

/**
 * @brief Converts a binary string to integer
 *
 * @param str
 * @return unsigned long long
 */
unsigned long long
BinaryToInt(char * str)
{
    size_t             Len;
    unsigned long long Acc = 0;

    Len = strlen(str);

    for (int i = 0; i < Len; i++)
    {
        Acc <<= 1;
        Acc += (str[i] - '0');
    }
    return Acc;
}

/**
 * @brief Rotate a character array to the left by one time
 *
 * @param str
 */
void
RotateLeftStringOnce(char * str)
{
    int  length = (int)strlen(str);
    char temp   = str[0];
    for (int i = 0; i < (length - 1); i++)
    {
        str[i] = str[i + 1];
    }
    str[length - 1] = temp;
}
