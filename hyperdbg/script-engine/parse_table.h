#ifndef PARSE_TABLE_H
#define PARSE_TABLE_H
#include "common.h"
#include "ScriptEngineCommonDefinitions.h"
#define RULES_COUNT                     73
#define TERMINAL_COUNT                  43
#define NONETERMINAL_COUNT              34
#define START_VARIABLE                  "S"
#define MAX_RHS_LEN                     8
#define KEYWORD_LIST_LENGTH             15
#define OPERATORS_LIST_LENGTH           10
#define REGISTER_MAP_LIST_LENGTH        31
#define PSEUDO_REGISTER_MAP_LIST_LENGTH 9
#define SEMANTIC_RULES_MAP_LIST_LENGTH  26
#define ONEOPFUNC1_LENGTH               9
#define ONEOPFUNC2_LENGTH               4
#define ZEROOPFUNC1_LENGTH              1
#define VARARGFUNC1_LENGTH              1
extern const struct _TOKEN Lhs[RULES_COUNT];
extern const struct _TOKEN Rhs[RULES_COUNT][MAX_RHS_LEN];
extern const unsigned int  RhsSize[RULES_COUNT];
extern const char *        NoneTerminalMap[NONETERMINAL_COUNT];
extern const char *        TerminalMap[TERMINAL_COUNT];
extern const int           ParseTable[NONETERMINAL_COUNT][TERMINAL_COUNT];
extern const char *        KeywordList[];
extern const char *        OperatorsList[];
extern const char *        OneOpFunc1[];
extern const char *        OneOpFunc2[];
extern const char *        ZeroOpFunc1[];
extern const char *        VarArgFunc1[];
extern const SYMBOL_MAP    SemanticRulesMapList[];
extern const SYMBOL_MAP    RegisterMapList[];
extern const SYMBOL_MAP    PseudoRegisterMapList[];

#define LALR_RULES_COUNT       22
#define LALR_TERMINAL_COUNT    10
#define LALR_NONTERMINAL_COUNT 14
#define LALR_MAX_RHS_LEN       3
#define LALR_STATE_COUNT       41
extern const struct _TOKEN LalrLhs[RULES_COUNT];
extern const struct _TOKEN LalrRhs[RULES_COUNT][MAX_RHS_LEN];
extern const unsigned int  LalrRhsSize[RULES_COUNT];
extern const char *        LalrNoneTerminalMap[NONETERMINAL_COUNT];
extern const char *        LalrTerminalMap[TERMINAL_COUNT];
extern const int           LalrGotoTable[LALR_STATE_COUNT][LALR_NONTERMINAL_COUNT];
extern const int           LalrActionTable[LALR_STATE_COUNT][LALR_NONTERMINAL_COUNT];
#endif
