/**
 * @file ScriptEngineCommon.h
 * @author M.H. Gholamrezei (gholamrezaei.mh@gmail.com)
 * @brief Shared Headers for Script engine
 * @details
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

typedef struct SYMBOL {
  long long unsigned Type;
  long long unsigned Value;
} SYMBOL, *PSYMBOL;

typedef struct SYMBOL_BUFFER {
  PSYMBOL Head;
  unsigned int Pointer;
  unsigned int Size;
  char* Message; 
  
} SYMBOL_BUFFER, *PSYMBOL_BUFFER;

#define FUNC_OR 0
#define FUNC_XOR 1
#define FUNC_AND 2
#define FUNC_ASR 3
#define FUNC_ASL 4
#define FUNC_ADD 5
#define FUNC_SUB 6
#define FUNC_MUL 7
#define FUNC_DIV 8
#define FUNC_MOD 9
#define FUNC_POI 10
#define FUNC_DB 11
#define FUNC_DD 12
#define FUNC_DW 13
#define FUNC_DQ 14
#define FUNC_STR 15
#define FUNC_WSTR 16
#define FUNC_SIZEOF 17
#define FUNC_NOT 18
#define FUNC_NEG 19
#define FUNC_HI 20
#define FUNC_LOW 21
#define FUNC_MOV 22

#define SYMBOL_ID_TYPE 0
#define SYMBOL_NUM_TYPE 1
#define SYMBOL_REGISTER_TYPE 2
#define SYMBOL_PSEUDO_REG_TYPE 3
#define SYMBOL_SEMANTIC_RULE_TYPE 4
#define SYMBOL_TEMP_TYPE 5

#define RAX_MNEMONIC 0
#define RCX_MNEMONIC 1
#define RDX_MNEMONIC 2
#define RBX_MNEMONIC 3
#define RSP_MNEMONIC 4
#define RBP_MNEMONIC 5
#define RSI_MNEMONIC 6
#define RDI_MNEMONIC 7
#define R8_MNEMONIC 8
#define R9_MNEMONIC 9
#define R10_MNEMONIC 10
#define R11_MNEMONIC 11
#define R12_MNEMONIC 12
#define R13_MNEMONIC 13
#define R14_MNEMONIC 14
#define R15_MNEMONIC 15

#define INVALID -1

#define TID_MNEMONIC 0
#define PID_MNEMONIC 1


#define SYMBOL_ID_TYPE  0
#define SYMBOL_NUM_TYPE 1
#define SYMBOL_REGISTER_TYPE 2
#define SYMBOL_PSEUDO_REG_TYPE 3
#define SYMBOL_SEMANTIC_RULE_TYPE 4
#define SYMBOL_TEMP 5

