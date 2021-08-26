#pragma once
#ifndef SCRIPT_ENGINE_COMMON_DEFINITIONS_H
#define SCRIPT_ENGINE_COMMON_DEFINITIONS_H
typedef struct SYMBOL {
	long long unsigned Type;
	long long unsigned Value;
} SYMBOL, * PSYMBOL;
typedef struct SYMBOL_BUFFER {
	PSYMBOL Head;
	unsigned int Pointer;
	unsigned int Size;
	char* Message;
} SYMBOL_BUFFER, * PSYMBOL_BUFFER;
typedef struct SYMBOL_MAP
{
    char* Name;
    long long unsigned Type;
} SYMBOL_MAP, * PSYMBOL_MAP;
typedef struct ACTION_BUFFER {
  long long unsigned Tag;
  long long unsigned CurrentAction;
  char ImmediatelySendTheResults;
  long long unsigned Context;
} ACTION_BUFFER, *PACTION_BUFFER;


#define SYMBOL_ID_TYPE 0
#define SYMBOL_NUM_TYPE 1
#define SYMBOL_REGISTER_TYPE 2
#define SYMBOL_PSEUDO_REG_TYPE 3
#define SYMBOL_SEMANTIC_RULE_TYPE 4
#define SYMBOL_TEMP_TYPE 5
#define SYMBOL_STRING_TYPE 6
#define SYMBOL_VARIABLE_COUNT_TYPE 7
#define SYMBOL_MEM_VALID_CHECK_MASK (1 << 31)
#define SYMBOL_INVALID 8
#define INVALID -99
#define LALR_ACCEPT 99



#define FUNC_INC 0
#define FUNC_DEC 1
#define FUNC_OR 2
#define FUNC_XOR 3
#define FUNC_AND 4
#define FUNC_ASR 5
#define FUNC_ASL 6
#define FUNC_ADD 7
#define FUNC_SUB 8
#define FUNC_MUL 9
#define FUNC_DIV 10
#define FUNC_MOD 11
#define FUNC_GT 12
#define FUNC_LT 13
#define FUNC_EGT 14
#define FUNC_ELT 15
#define FUNC_EQUAL 16
#define FUNC_NEQ 17
#define FUNC_START_OF_IF 18
#define FUNC_JMP 19
#define FUNC_JZ 20
#define FUNC_JNZ 21
#define FUNC_JMP_TO_END_AND_JZCOMPLETED 22
#define FUNC_END_OF_IF 23
#define FUNC_START_OF_WHILE 24
#define FUNC_END_OF_WHILE 25
#define FUNC_VARGSTART 26
#define FUNC_MOV 27
#define FUNC_START_OF_DO_WHILE 28
#define FUNC_ 29
#define FUNC_START_OF_DO_WHILE_COMMANDS 30
#define FUNC_END_OF_DO_WHILE 31
#define FUNC_START_OF_FOR 32
#define FUNC_FOR_INC_DEC 33
#define FUNC_START_OF_FOR_OMMANDS 34
#define FUNC_END_OF_IF 35
#define FUNC_IGNORE_LVALUE 36
#define FUNC_PRINT 37
#define FUNC_FORMATS 38
#define FUNC_DISABLE_EVENT 39
#define FUNC_ENABLE_EVENT 40
#define FUNC_TEST_STATEMENT 41
#define FUNC_PRINTF 42
#define FUNC_PAUSE 43
#define FUNC_ED 44
#define FUNC_EB 45
#define FUNC_EQ 46
#define FUNC_POI 47
#define FUNC_DB 48
#define FUNC_DD 49
#define FUNC_DW 50
#define FUNC_DQ 51
#define FUNC_NEG 52
#define FUNC_HI 53
#define FUNC_LOW 54
#define FUNC_NOT 55
#define FUNC_CHECK_ADDRESS 56
#define FUNC_ED 57
#define FUNC_EB 58
#define FUNC_EQ 59
typedef enum REGS_ENUM {
	REGISTER_RAX = 0,
	REGISTER_RCX = 1,
	REGISTER_RDX = 2,
	REGISTER_RBX = 3,
	REGISTER_RSP = 4,
	REGISTER_RBP = 5,
	REGISTER_RSI = 6,
	REGISTER_RDI = 7,
	REGISTER_R8 = 8,
	REGISTER_R9 = 9,
	REGISTER_R10 = 10,
	REGISTER_R11 = 11,
	REGISTER_R12 = 12,
	REGISTER_R13 = 13,
	REGISTER_R14 = 14,
	REGISTER_R15 = 15,
	REGISTER_DS = 16,
	REGISTER_ES = 17,
	REGISTER_FS = 18,
	REGISTER_GS = 19,
	REGISTER_CS = 20,
	REGISTER_SS = 21,
	REGISTER_RFLAGS = 22,
	REGISTER_RIP = 23,
	REGISTER_IDTR = 24,
	REGISTER_GDTR = 25,
	REGISTER_CR0 = 26,
	REGISTER_CR2 = 27,
	REGISTER_CR3 = 28,
	REGISTER_CR4 = 29,
	REGISTER_CR8 = 30,
	REGISTER_EAX = 31,
	REGISTER_AX = 32,
	REGISTER_AH = 33,
	REGISTER_AL = 34,
	REGISTER_EBX = 35,
	REGISTER_BX = 36,
	REGISTER_BH = 37,
	REGISTER_BL = 38,
	REGISTER_ECX = 39,
	REGISTER_CX = 40,
	REGISTER_CH = 41,
	REGISTER_CL = 42,
	REGISTER_EDX = 43,
	REGISTER_DX = 44,
	REGISTER_DH = 45,
	REGISTER_DL = 46,
	REGISTER_ESP = 47,
	REGISTER_SP = 48,
	REGISTER_SPL = 49,
	REGISTER_EBP = 50,
	REGISTER_BP = 51,
	REGISTER_BPL = 52,
	REGISTER_ESI = 53,
	REGISTER_SI = 54,
	REGISTER_SIL = 55,
	REGISTER_EDI = 56,
	REGISTER_DI = 57,
	REGISTER_DIL = 58,
	REGISTER_R8D = 59,
	REGISTER_R8W = 60,
	REGISTER_R8H = 61,
	REGISTER_R8L = 62,
	REGISTER_R9D = 63,
	REGISTER_R9W = 64,
	REGISTER_R9H = 65,
	REGISTER_R9L = 66,
	REGISTER_R10D = 67,
	REGISTER_R10W = 68,
	REGISTER_R10H = 69,
	REGISTER_R10L = 70,
	REGISTER_R11D = 71,
	REGISTER_R11W = 72,
	REGISTER_R11H = 73,
	REGISTER_R11L = 74,
	REGISTER_R12D = 75,
	REGISTER_R12W = 76,
	REGISTER_R12H = 77,
	REGISTER_R12L = 78,
	REGISTER_R13D = 79,
	REGISTER_R13W = 80,
	REGISTER_R13H = 81,
	REGISTER_R13L = 82,
	REGISTER_R14D = 83,
	REGISTER_R14W = 84,
	REGISTER_R14H = 85,
	REGISTER_R14L = 86,
	REGISTER_R15D = 87,
	REGISTER_R15W = 88,
	REGISTER_R15H = 89,
	REGISTER_R15L = 90,
	REGISTER_EFLAGS = 91,
	REGISTER_FLAGS = 92,
	REGISTER_EIP = 93,
	REGISTER_IP = 94

} REGS_ENUM;

static const char *const RegistersNames[] = {
"rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
"r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
"ds", "es", "fs", "gs", "cs", "ss", "rflags", "rip",
"idtr", "gdtr", "cr0", "cr2", "cr3", "cr4", "cr8", "eax",
"ax", "ah", "al", "ebx", "bx", "bh", "bl", "ecx",
"cx", "ch", "cl", "edx", "dx", "dh", "dl", "esp",
"sp", "spl", "ebp", "bp", "bpl", "esi", "si", "sil",
"edi", "di", "dil", "r8d", "r8w", "r8h", "r8l", "r9d",
"r9w", "r9h", "r9l", "r10d", "r10w", "r10h", "r10l", "r11d",
"r11w", "r11h", "r11l", "r12d", "r12w", "r12h", "r12l", "r13d",
"r13w", "r13h", "r13l", "r14d", "r14w", "r14h", "r14l", "r15d",
"r15w", "r15h", "r15l", "eflags", "flags", "eip", 	"ip"
};

#define PSEUDO_REGISTER_PID 0
#define PSEUDO_REGISTER_TID 1
#define PSEUDO_REGISTER_PROC 2
#define PSEUDO_REGISTER_THREAD 3
#define PSEUDO_REGISTER_PEB 4
#define PSEUDO_REGISTER_TEB 5
#define PSEUDO_REGISTER_IP 6
#define PSEUDO_REGISTER_BUFFER 7
#define PSEUDO_REGISTER_CONTEXT 8

#endif
