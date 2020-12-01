#ifndef PARSE_TABLE_H
#define PARSE_TABLE_H
#define RULES_COUNT 55
#define TERMINAL_COUNT 35
#define NONETERMINAL_COUNT 25
#define START_VARIABLE "S"
#define MAX_RHS_LEN 6
#define KEYWORD_LIST_LENGTH 12
extern const struct _TOKEN Lhs[RULES_COUNT];
extern const struct _TOKEN Rhs[RULES_COUNT][MAX_RHS_LEN];
extern const unsigned int RhsSize[RULES_COUNT];
const char* NoneTerminalMap[NONETERMINAL_COUNT];
const char* TerminalMap[TERMINAL_COUNT];
const int ParseTable[NONETERMINAL_COUNT][TERMINAL_COUNT];
extern const char* KeywordList[];
#endif
