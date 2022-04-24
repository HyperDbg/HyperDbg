#include "pch.h"

/**
 * @brief allocates a new token
 *
 * @return Token
 */
TOKEN
NewToken()
{
    TOKEN Token;

    //
    // Allocates memory for token and its value
    //
    Token        = (TOKEN)malloc(sizeof(*Token));
    Token->Value = (char *)calloc(TOKEN_VALUE_MAX_LEN + 1, sizeof(char));

    //
    // Init fields
    //
    strcpy(Token->Value, "");
    Token->Type    = UNKNOWN;
    Token->len     = 0;
    Token->max_len = TOKEN_VALUE_MAX_LEN;

    return Token;
}

/**
 * @brief removes allocated memory of a token
 *
 * @param Token
 */
void
RemoveToken(TOKEN Token)
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
void
PrintToken(TOKEN Token)
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
    case TEMP:
        printf(" TEMP>\n");
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
void
Append(TOKEN Token, char c)
{
    //
    // Check overflow of the string
    //
    if (Token->len >= Token->max_len - 1)
    {
        //
        // Double the length of the allocated space for the string
        //
        Token->max_len *= 2;
        char * NewValue = (char *)calloc(Token->max_len + 1, sizeof(char));

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
TOKEN_LIST
NewTokenList(void)
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
    TokenList->Size    = TOKEN_LIST_INIT_SIZE;

    //
    // Allocation of memory for TOKEN_LIST buffer
    //
    TokenList->Head = (TOKEN *)malloc(TokenList->Size * sizeof(TOKEN));

    return TokenList;
}

/**
 * @brief removes allocated memory of a TOKEN_LIST
 *
 * @param TokenList
 */
void
RemoveTokenList(TOKEN_LIST TokenList)
{
    TOKEN Token;
    for (uintptr_t i = 0; i < TokenList->Pointer; i++)
    {
        Token = *(TokenList->Head + i);
        RemoveToken(Token);
    }
    free(TokenList->Head);
    free(TokenList);

    return;
}

/**
 * @brief prints each Token inside a TokenList
 *
 * @param TokenList
 */
void
PrintTokenList(TOKEN_LIST TokenList)
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
TOKEN_LIST
Push(TOKEN_LIST TokenList, TOKEN Token)
{
    //
    // Calculate address to write new token
    //
    uintptr_t Head      = (uintptr_t)TokenList->Head;
    uintptr_t Pointer   = (uintptr_t)TokenList->Pointer;
    TOKEN *   WriteAddr = (TOKEN *)(Head + Pointer * sizeof(TOKEN));

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
        TOKEN * NewHead = (TOKEN *)malloc(2 * TokenList->Size * sizeof(TOKEN));

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
TOKEN
Pop(TOKEN_LIST TokenList)
{
    //
    // Calculate address to read most recent token
    //
    if (TokenList->Pointer > 0)
        TokenList->Pointer--;
    uintptr_t Head     = (uintptr_t)TokenList->Head;
    uintptr_t Pointer  = (uintptr_t)TokenList->Pointer;
    TOKEN *   ReadAddr = (TOKEN *)(Head + Pointer * sizeof(TOKEN));

    return *ReadAddr;
}

/**
 * @brief returns last Token of a TokenList
 *
 * @param TokenList
 * @return Token
 */
TOKEN
Top(TOKEN_LIST TokenList)
{
    //
    // Calculate address to read most recent pushed token
    //
    uintptr_t Head     = (uintptr_t)TokenList->Head;
    uintptr_t Pointer  = (uintptr_t)TokenList->Pointer - 1;
    TOKEN *   ReadAddr = (TOKEN *)(Head + Pointer * sizeof(TOKEN));

    return *ReadAddr;
}

/**
 * @brief cheks whether input char belongs to hexadecimal digit-set or not
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
 * @brief cheks whether input char belongs to decimal digit-set or not
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
 * @brief cheks whether input char belongs to alphabet set or not
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
 * @brief cheks whether input char belongs to binary digit-set or not
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
 * @brief cheks whether input char belongs to octal digit-set or not
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

TOKEN
NewTemp(PSCRIPT_ENGINE_ERROR_TYPE Error)
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
    TOKEN Temp = NewToken();
    char  TempValue[8];
    sprintf(TempValue, "%d", TempID);
    strcpy(Temp->Value, TempValue);
    Temp->Type = TEMP;
    return Temp;
}
void
FreeTemp(TOKEN Temp)
{
    int id = DecimalToInt(Temp->Value);
    if (Temp->Type == TEMP)
    {
        TempMap[id] = 0;
    }
}

void
CleanTempList(void)
{
    for (int i = 0; i < MAX_TEMP_COUNT; i++)
    {
        TempMap[i] = 0;
    }
}

char
IsType1Func(TOKEN Operator)
{
    unsigned int n = ONEOPFUNC1_LENGTH;
    for (int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, OneOpFunc1[i]))
        {
            return 1;
        }
    }
    return 0;
}

char
IsType2Func(TOKEN Operator)
{
    unsigned int n = ONEOPFUNC2_LENGTH;
    for (int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, OneOpFunc2[i]))
        {
            return 1;
        }
    }
    return 0;
}

char
IsTwoOperandOperator(TOKEN Operator)
{
    unsigned int n = OPERATORS_TWO_OPERAND_LIST_LENGTH;
    for (int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, OperatorsTwoOperandList[i]))
        {
            return 1;
        }
    }
    return 0;
}

char
IsOneOperandOperator(TOKEN Operator)
{
    unsigned int n = OPERATORS_ONE_OPERAND_LIST_LENGTH;
    for (int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, OperatorsOneOperandList[i]))
        {
            return 1;
        }
    }
    return 0;
}

char
IsType4Func(TOKEN Operator)
{
    unsigned int n = VARARGFUNC1_LENGTH;
    for (int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, VarArgFunc1[i]))
        {
            return 1;
        }
    }
    return 0;
}

char
IsType5Func(TOKEN Operator)
{
    unsigned int n = ZEROOPFUNC1_LENGTH;
    for (int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, ZeroOpFunc1[i]))
        {
            return 1;
        }
    }
    return 0;
}

char
IsType6Func(TOKEN Operator)
{
    unsigned int n = TWOOPFUNC1_LENGTH;
    for (int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, TwoOpFunc1[i]))
        {
            return 1;
        }
    }
    return 0;
}

char
IsType7Func(TOKEN Operator)
{
    unsigned int n = TWOOPFUNC2_LENGTH;
    for (int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, TwoOpFunc2[i]))
        {
            return 1;
        }
    }
    return 0;
}

char
IsType8Func(TOKEN Operator)
{
    unsigned int n = THREEOPFUNC1_LENGTH;
    for (int i = 0; i < n; i++)
    {
        if (!strcmp(Operator->Value, ThreeOpFunc1[i]))
        {
            return 1;
        }
    }
    return 0;
}

/**
 *
 *
 *
 */
char
IsNoneTerminal(TOKEN Token)
{
    if (Token->Value[0] >= 'A' && Token->Value[0] <= 'Z')
        return 1;
    else
        return 0;
}

/**
 *
 *
 *
 */
char
IsSemanticRule(TOKEN Token)
{
    if (Token->Value[0] == '@')
        return 1;
    else
        return 0;
}

/**
 *
 *
 *
 */
int
GetNonTerminalId(TOKEN Token)
{
    for (int i = 0; i < NONETERMINAL_COUNT; i++)
    {
        if (!strcmp(Token->Value, NoneTerminalMap[i]))
            return i;
    }
    return INVALID;
}

/**
 *
 *
 *
 */
int
GetTerminalId(TOKEN Token)
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

        else // Keyword
        {
            if (!strcmp(Token->Value, TerminalMap[i]))
                return i;
        }
    }
    return INVALID;
}

/**
 *
 *
 *
 */
int
LalrGetNonTerminalId(TOKEN Token)
{
    for (int i = 0; i < LALR_NONTERMINAL_COUNT; i++)
    {
        if (!strcmp(Token->Value, LalrNoneTerminalMap[i]))
            return i;
    }
    return INVALID;
}

/**
 *
 *
 *
 */
int
LalrGetTerminalId(TOKEN Token)
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

        else // Keyword
        {
            if (!strcmp(Token->Value, LalrTerminalMap[i]))
                return i;
        }
    }
    return INVALID;
}

/**
 *
 *
 *
 */
char
IsEqual(const TOKEN Token1, const TOKEN Token2)
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

void
SetType(unsigned long long * Val, unsigned char Type)
{
    *Val = (unsigned long long int)Type;
}

unsigned long long int
DecimalToInt(char * str)
{
    unsigned long long int Acc = 0;
    size_t                 Len;

    Len = strlen(str);
    for (int i = 0; i < Len; i++)
    {
        Acc *= 10;
        Acc += (str[i] - '0');
    }
    return Acc;
}

unsigned long long int
DecimalToSignedInt(char * str)
{
    long long int Acc = 0;
    size_t        Len;

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

unsigned long long int
HexToInt(char * str)
{
    char                   temp;
    size_t                 len = strlen(str);
    unsigned long long int Acc = 0;
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
unsigned long long int
OctalToInt(char * str)
{
    size_t                 Len;
    unsigned long long int Acc = 0;

    Len = strlen(str);

    for (int i = 0; i < Len; i++)
    {
        Acc <<= 3;
        Acc += (str[i] - '0');
    }
    return Acc;
}
unsigned long long int
BinaryToInt(char * str)
{
    size_t                 Len;
    unsigned long long int Acc = 0;

    Len = strlen(str);

    for (int i = 0; i < Len; i++)
    {
        Acc <<= 1;
        Acc += (str[i] - '0');
    }
    return Acc;
}
