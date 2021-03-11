from ll1_parser import *
from lalr1_parser import *

class Generator():
    def __init__(self): 
        self.SourceFile = open("..\\parse_table.c", "w")
        self.HeaderFile = open("..\\parse_table.h", "w")   
        self.CommonHeaderFile = open("..\\..\\include\\ScriptEngineCommonDefinitions.h", "w")
        self.ll1 = LL1Parser(self.SourceFile, self.HeaderFile, self.CommonHeaderFile)
        self.lalr = LALR1Parser(self.SourceFile, self.HeaderFile)

    def Run(self):     

          
        self.WriteCommonHeader()
        self.ll1.Run()
        self.HeaderFile.write("\n\n")
        self.lalr.ReadGrammar()
        
        self.gr = get_grammar(self.lalr)
        self.lalr_table = lalr_one.ParsingTable(self.gr)
        self.lalr.ParseTable = self.lalr_table
        self.ll1.SetLalr(self.lalr, self.lalr_table)

        self.lalr.Run()

        self.CommonHeaderFile.write("#endif\n")


        # Closes Output Files 
        self.SourceFile.close()
        self.HeaderFile.close()
        self.CommonHeaderFile.close()


    def WriteCommonHeader(self):
        self.CommonHeaderFile.write(
         """#pragma once
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
#define INVALID -99

\n\n""")

    

    def Parse(self, Tokens):
        Stack = self.ll1.Parse(Tokens)
        print(Stack)    
        

if __name__ == "__main__": 
   
    gen = Generator()
    gen.Run()


    Tokens = ['if', '(','_id', '<', '_id', '&&', '_id', '>', '_id', ')', '{', 'print', '(', '+', '_hex',  ')', ';', '}', '$']
    gen.Parse(Tokens)
