#pragma once
#ifndef PARSE_TABLE_H
#define PARSE_TABLE_H
#define RULES_COUNT 134
#define TERMINAL_COUNT 73
#define NONETERMINAL_COUNT 34
#define START_VARIABLE "S"
#define MAX_RHS_LEN 15
#define KEYWORD_LIST_LENGTH 58
#define OPERATORS_ONE_OPERAND_LIST_LENGTH 4
#define OPERATORS_TWO_OPERAND_LIST_LENGTH 16
#define REGISTER_MAP_LIST_LENGTH 120
#define PSEUDO_REGISTER_MAP_LIST_LENGTH 13
#define SEMANTIC_RULES_MAP_LIST_LENGTH 97
#define THREEOPFUNC1_LENGTH 1
#define TWOOPFUNC1_LENGTH 5
#define TWOOPFUNC2_LENGTH 1
#define ONEOPFUNC1_LENGTH 17
#define ONEOPFUNC2_LENGTH 7
#define ZEROOPFUNC1_LENGTH 3
#define VARARGFUNC1_LENGTH 1
extern const struct _TOKEN Lhs[RULES_COUNT];
extern const struct _TOKEN Rhs[RULES_COUNT][MAX_RHS_LEN];
extern const unsigned int RhsSize[RULES_COUNT];
extern const char* NoneTerminalMap[NONETERMINAL_COUNT];
extern const char* TerminalMap[TERMINAL_COUNT];
extern const int ParseTable[NONETERMINAL_COUNT][TERMINAL_COUNT];
extern const char* KeywordList[];
extern const char* OperatorsTwoOperandList[];
extern const char* OperatorsOneOperandList[];
extern const char* ThreeOpFunc1[];
extern const char* TwoOpFunc1[];
extern const char* TwoOpFunc2[];
extern const char* OneOpFunc1[];
extern const char* OneOpFunc2[];
extern const char* ZeroOpFunc1[];
extern const char* VarArgFunc1[];
extern const SYMBOL_MAP SemanticRulesMapList[];
extern const SYMBOL_MAP RegisterMapList[];
extern const SYMBOL_MAP PseudoRegisterMapList[];


#define LALR_RULES_COUNT 69
#define LALR_TERMINAL_COUNT 54
#define LALR_NONTERMINAL_COUNT 15
#define LALR_MAX_RHS_LEN 9
#define LALR_STATE_COUNT 180
extern const struct _TOKEN LalrLhs[RULES_COUNT];
extern const struct _TOKEN LalrRhs[RULES_COUNT][MAX_RHS_LEN];
extern const unsigned int LalrRhsSize[RULES_COUNT];
extern const char* LalrNoneTerminalMap[NONETERMINAL_COUNT];
extern const char* LalrTerminalMap[TERMINAL_COUNT];
extern const int LalrGotoTable[LALR_STATE_COUNT][LALR_NONTERMINAL_COUNT];
extern const int LalrActionTable[LALR_STATE_COUNT][LALR_TERMINAL_COUNT];
extern const struct _TOKEN LalrSemanticRules[RULES_COUNT];
#endif
