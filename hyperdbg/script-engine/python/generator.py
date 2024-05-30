"""
 * @file generator.py
 * @author M.H. Gholamrezei (mh@hyperdbg.org)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Script engine LL(1) Parse table generator 
 * @details This program reads grammar from Greammar.txt file 
 *          placed in the same directory of the program 
 *          and creates parse_table.h and parse_table.c which is 
 *          used by the parser of script engine. 
 * @version 0.1
 * @date 2020-10-24
 *
 * @copyright This project is released under the GNU Public License v3.
 
 """

from ll1_parser import *
from lalr1_parser import *

class Generator():
    def __init__(self): 
        self.SourceFile = open("..\\code\\parse-table.c", "w")
        self.HeaderFile = open("..\\header\\parse-table.h", "w")   
        self.CommonHeaderFile = open("..\\..\\script-eval\\header\\ScriptEngineCommonDefinitions.h", "w")
        self.CommonHeaderFileScala = open("..\\..\\..\\hwdbg\\src\\main\\scala\\hwdbg\\script\\script_definitions.scala", "w")
        self.ll1 = LL1Parser(self.SourceFile, self.HeaderFile, self.CommonHeaderFile, self.CommonHeaderFileScala)
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
    
        #
        # Write scala headers
        #
        self.CommonHeaderFileScala.write("""package hwdbg.script

import chisel3._
import chisel3.util._

/**
 * @brief
 *   The structure of SYMBOL used in script engine of HyperDbg
 */
class Symbol extends Bundle {
  val Type = UInt(64.W) // long long unsigned is 64 bits
  val Len = UInt(64.W)
  val VariableType = UInt(64.W)
  val Value = UInt(64.W)
}

/**
 * @brief
 *   Constant values for the script engine
 */
object ScriptConstants {
  val SYMBOL_MEM_VALID_CHECK_MASK = 1 << 31
  val INVALID = 0x80000000
  val LALR_ACCEPT = 0x7fffffff
}

/**
 * @brief
 *   Constant type values for the script engine
 */
object ScriptConstantTypes {
  object ScriptDataTypes extends ChiselEnum {
    val symbolGlobalIdType, symbolLocalIdType, symbolNumType, symbolRegisterType, symbolPseudoRegType, symbolSemanticRuleType, symbolTempType, symbolStringType, symbolVariableCountType, symbolInvalid, symbolWstringType, symbolUserDefinedFunctionType, symbolFunctionParameterIdType, symbolReturnAddressType, symbolStackTempType, symbolFunctionParameterType  = Value
  }
}

""")
        #
        # Write C/C++ headers
        #
        self.CommonHeaderFile.write(
         """#pragma once
#ifndef SCRIPT_ENGINE_COMMON_DEFINITIONS_H
#define SCRIPT_ENGINE_COMMON_DEFINITIONS_H
typedef struct SYMBOL {
	long long unsigned Type;
    long long unsigned Len;
    long long unsigned VariableType;
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
  char CallingStage;
} ACTION_BUFFER, *PACTION_BUFFER;


#define SYMBOL_GLOBAL_ID_TYPE 0
#define SYMBOL_LOCAL_ID_TYPE 1
#define SYMBOL_NUM_TYPE 2
#define SYMBOL_REGISTER_TYPE 3
#define SYMBOL_PSEUDO_REG_TYPE 4
#define SYMBOL_SEMANTIC_RULE_TYPE 5
#define SYMBOL_TEMP_TYPE 6
#define SYMBOL_STRING_TYPE 7
#define SYMBOL_VARIABLE_COUNT_TYPE 8
#define SYMBOL_INVALID 9
#define SYMBOL_WSTRING_TYPE 10
#define SYMBOL_USER_DEFINED_FUNCTION_TYPE 11
#define SYMBOL_FUNCTION_PARAMETER_ID_TYPE 12
#define SYMBOL_RETURN_ADDRESS_TYPE 13
#define SYMBOL_STACK_TEMP_TYPE 14
#define SYMBOL_FUNCTION_PARAMETER_TYPE 15

#define SYMBOL_MEM_VALID_CHECK_MASK (1 << 31)
#define INVALID 0x80000000
#define LALR_ACCEPT 0x7fffffff

\n\n""")

    

    def Parse(self, Tokens):
        Stack = self.ll1.Parse(Tokens)
        print(Stack)    
        

if __name__ == "__main__": 
   
    gen = Generator()
    gen.Run()


    Tokens = ['print', '(', '+', '_hex',  ')', ';', '$']
    gen.Parse(Tokens)
