#pragma once
#ifndef PARSE_TABLE_H
#define PARSE_TABLE_H
#define RULES_COUNT 241
#define TERMINAL_COUNT 116
#define NONETERMINAL_COUNT 49
#define START_VARIABLE "S"
#define MAX_RHS_LEN 15
#define KEYWORD_LIST_LENGTH 104
#define OPERATORS_ONE_OPERAND_LIST_LENGTH 4
#define OPERATORS_TWO_OPERAND_LIST_LENGTH 16
#define REGISTER_MAP_LIST_LENGTH 120
#define PSEUDO_REGISTER_MAP_LIST_LENGTH 16
#define SCRIPT_VARIABLE_TYPE_LIST_LENGTH 10
#define ASSIGNMENT_OPERATOR_LIST_LENGTH 10
#define SEMANTIC_RULES_MAP_LIST_LENGTH 150
#define THREEOPFUNC1_LENGTH 1
#define THREEOPFUNC2_LENGTH 3
#define TWOOPFUNC1_LENGTH 8
#define TWOOPFUNC2_LENGTH 2
#define ONEOPFUNC1_LENGTH 25
#define ONEOPFUNC2_LENGTH 9
#define ONEOPFUNC3_LENGTH 1
#define TWOOPFUNC3_LENGTH 1
#define THREEOPFUNC3_LENGTH 2
#define THREEOPFUNC4_LENGTH 1
#define ONEOPFUNC4_LENGTH 1
#define TWOOPFUNC4_LENGTH 1
#define ZEROOPFUNC1_LENGTH 7
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
extern const char* AssignmentOperatorList[];
extern const char* ThreeOpFunc1[];
extern const char* ThreeOpFunc2[];
extern const char* TwoOpFunc1[];
extern const char* TwoOpFunc2[];
extern const char* OneOpFunc1[];
extern const char* OneOpFunc2[];
extern const char* OneOpFunc3[];
extern const char* TwoOpFunc3[];
extern const char* ThreeOpFunc3[];
extern const char* ThreeOpFunc4[];
extern const char* OneOpFunc4[];
extern const char* TwoOpFunc4[];
extern const char* ZeroOpFunc1[];
extern const char* VarArgFunc1[];
extern const SYMBOL_MAP SemanticRulesMapList[];
extern const SYMBOL_MAP RegisterMapList[];
extern const SYMBOL_MAP PseudoRegisterMapList[];
extern const char* ScriptVariableTypeList[];


#define LALR_RULES_COUNT 103
#define LALR_TERMINAL_COUNT 76
#define LALR_NONTERMINAL_COUNT 22
#define LALR_MAX_RHS_LEN 9
#define LALR_STATE_COUNT 298
extern const struct _TOKEN LalrLhs[RULES_COUNT];
extern const struct _TOKEN LalrRhs[RULES_COUNT][MAX_RHS_LEN];
extern const unsigned int LalrRhsSize[RULES_COUNT];
extern const char* LalrNoneTerminalMap[NONETERMINAL_COUNT];
extern const char* LalrTerminalMap[TERMINAL_COUNT];
extern const int LalrGotoTable[LALR_STATE_COUNT][LALR_NONTERMINAL_COUNT];
extern const int LalrActionTable[LALR_STATE_COUNT][LALR_TERMINAL_COUNT];
extern const struct _TOKEN LalrSemanticRules[RULES_COUNT];
#endif
