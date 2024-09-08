"""
 * @file ll1_parse_table_generator.py
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

from util import *
from lalr1_parser import *

class LL1Parser:
    def __init__(self, SourceFile, HeaderFile, CommonHeaderFile, CommonHeaderFileScala):
        # The file which contains the grammar of the language 
        self.GrammarFile = open("Grammar.txt", "r")

        # The file which is used by parser for parsing the input 
        self.SourceFile = SourceFile
        self.HeaderFile = HeaderFile
        self.CommonHeaderFile = CommonHeaderFile 
        self.CommonHeaderFileScala = CommonHeaderFileScala 
        

        # Lists which used for storing the rules:
        # Right Hand Side(Rhs)
        self.RhsList = []

        # Left Hand Side(Lhs)
        self.LhsList = []

        # Set of all terminals and noneterminals
        self.TerminalSet = set()
        self.NonTerminalSet = set()

        # Start variable 
        self.Start = ""


        # maximum of "Right Hand Side(Rhs)" length
        self.MAXIMUM_RHS_LEN = 0


        self.SPECIAL_TOKENS = ['%', '+', '~', '++', '-', '--', "*", "/", "=", "==", "!=", ",", ";", "(", ")", "{", "}", "|", "||", ">>", ">=", "<<", "<=", "&", "&&", "^",
                              "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=" ]

        # INVALID rule indicator
        self.INVALID = 0x80000000

        self.FunctionsDict = dict()
        self.OperatorsTwoOperand = []
        self.OperatorsOneOperand = []
        self.RegistersList = []
        self.PseudoRegistersList = []
        self.VariableTypeList = []
        self.keywordList = []
        self.SemantiRulesList = []
        self.AssignmentOperator = []


        # Dictionaries used for storing first and follow sets 
        self.FirstDict = dict()
        self.FollowDict = dict()



    def Run(self):
        # Read grammar from input file and initialize grammar related variables 
        self.ReadGrammar()

        # Calculate "First Set" for all nonterminals and print it
        self.FindAllFirsts()
        # print("Firsts:")
        # self.PrintFirsts()
        # print("________________________________________________________________________________")

        # Calculate "Follow Set" for all nonterminals and print it
        self.FindAllFollows()
        # print("Follows:")
        # self.PrintFollows()
        # print("________________________________________________________________________________")
        
        # Calculate "Prdicted Set" for each rule and print it
        self.FindAllPredicts()
        # print("Predicts:")
        # self.print_predicts()
        # print("________________________________________________________________________________")

        # Fills "Parse Table" according to calculated "Predicted Set" and print "Parse Table"
        self.FillParseTable()
        # print("Parse Table:")
        # self.PrintParseTable()
        # print()
        
        # Prints variables that are needed for parser for parsing into the output file 
        self.HeaderFile.write("#pragma once\n")

        self.HeaderFile.write("#ifndef PARSE_TABLE_H\n")
        self.HeaderFile.write("#define PARSE_TABLE_H\n")
        
        self.HeaderFile.write("#define RULES_COUNT " + str(len(self.LhsList)) + "\n")
        self.HeaderFile.write("#define TERMINAL_COUNT " + str(len(list(self.TerminalSet))) + "\n")
        self.HeaderFile.write("#define NONETERMINAL_COUNT " + str(len(list(self.NonTerminalList))) + "\n")
        self.HeaderFile.write("#define START_VARIABLE " + "\"" + self.Start +"\"\n")
        self.HeaderFile.write("#define MAX_RHS_LEN "  + str(self.MAXIMUM_RHS_LEN) +"\n")
        self.HeaderFile.write("#define KEYWORD_LIST_LENGTH "  + str(len(self.keywordList)) +"\n")
        self.HeaderFile.write("#define OPERATORS_ONE_OPERAND_LIST_LENGTH " + str(len(self.OperatorsOneOperand)) + "\n")
        self.HeaderFile.write("#define OPERATORS_TWO_OPERAND_LIST_LENGTH " + str(len(self.OperatorsTwoOperand)) + "\n")
        self.HeaderFile.write("#define REGISTER_MAP_LIST_LENGTH " + str(len(self.RegistersList))+ "\n")
        self.HeaderFile.write("#define PSEUDO_REGISTER_MAP_LIST_LENGTH " + str(len(self.PseudoRegistersList))+ "\n")
        self.HeaderFile.write("#define SCRIPT_VARIABLE_TYPE_LIST_LENGTH " + str(len(self.VariableTypeList))+ "\n")
        self.HeaderFile.write("#define ASSIGNMENT_OPERATOR_LIST_LENGTH " + str(len(self.AssignmentOperator)) + "\n")
        self.HeaderFile.write("#define SEMANTIC_RULES_MAP_LIST_LENGTH " + str(len(self.keywordList) + len(self.OperatorsOneOperand) + len(self.OperatorsTwoOperand) + len(self.SemantiRulesList) + len(self.AssignmentOperator))+ "\n")
        for Key in self.FunctionsDict:
            self.HeaderFile.write("#define "+ Key[1:].upper() + "_LENGTH "+ str(len(self.FunctionsDict[Key]))+"\n")

       
        self.SourceFile.write("#include \"pch.h\"\n")



        # Prints Rules into output files
        self.WriteLhsList()
        self.WriteRhsList()
        
        # Prints size of each Rhs into output files
        self.WriteRhsSize()

        # Prints noneterminals and Terminal into output files
        self.WriteNoneTermianlList()
        self.WriteTerminalList()

        # Prints "Parse Table" into output files
        self.WriteParseTable()

        # Prints Keywords list into output files 
        self.WriteKeywordList()

        # Prints Operators List into output files 
        self.WriteOperatorsList()

        # Prints Maps into output files 
        self.WriteMaps()
        

        

        self.WriteSemanticMaps()
        self.WriteRegisterMaps()
        self.WritePseudoRegMaps()
        self.WriteVariableTypeList()

        # Closes Grammar Input File 
        self.GrammarFile.close()

 


    def SetLalr(self, Lalr, LalrParseTable):
        self.Lalr = Lalr
        self.LalrParseTable = LalrParseTable

    # This function simulates of script engine parser in ScriptEngine.C in
    # order to test the generated "Parse Table"
    def Parse(self, Tokens):
        # Initialize Parse Stack 
        Stack = []

        # Initialize Match Stack 
        MatchedStack = []

        # Push the end of stack indicator into stack
        Stack.append("$")

        # Push start variable into stack
        Stack.append(self.Start)

        # Assign top variale an invalid value 
        Top = ""

        # Read input
        Tokens, CurrentIn = Read(Tokens)

        # Temporary values counter initialized with 0 value 
        TempCounter = 0 
        
        # While Stack is not empty repeat 
        while Top != "$":

            # Read top of stack 
            Top = GetTop(Stack)
            # print(Stack)
            # print("Top:", Top)
            # print("CurrentIn:", CurrentIn, "\n\n")
            # x = input()


            
            if self.IsNoneTerminal(Top):
                if Top == "BOOLEAN_EXPRESSION":
                    print("Top == BOOLEAN_EXPRESSION:")
                    print(Stack)
                    print(Tokens)
                    print(CurrentIn)
                    print("=====================================\n\n")

                    Stack.pop()
                    
                    #------------------------------------------------------
                    # Get BE Tokens 
                    BETokensSize = 0
                    OpenParanthesesCount = 1 

                    i = 0
                    while True:
                        TempToken = Tokens[i]
                       
                        i +=1
                        if TempToken == '(' :
                            OpenParanthesesCount += 1
                            BETokensSize += 1
                        elif TempToken == ')':
                            OpenParanthesesCount -= 1
                            if OpenParanthesesCount == 0:
                                break 
                            else:
                                BETokensSize +=1
                        else:
                            BETokensSize +=1
                        print("TempToken: ", end = "")
                        print(TempToken)
                        print("BETokensSize: ", end = "")
                        print(BETokensSize)


                    # BETokens.append("$end")
                    print("BETokensSize: ", end = "")
                    print(BETokensSize)
                    y = input()
                    NewTokens = [CurrentIn]
                    for x in Tokens:
                        NewTokens.append(x)
                    #------------------------------------------------------
                    Tokens = self.Lalr.Parse(NewTokens, BETokensSize + 1)
                    # x = input()
                    print("after lalr parsing:")
                    print(Stack)
                    print(Tokens)
                    print("=====================================\n\n")
                    

                    CurrentIn = Tokens[0]   
                    if (len(Tokens) > 1):
                        Tokens = Tokens[1:]
                    # x = input()
                else:
                    Id = self.ParseTable[self.GetNoneTerminalId(Top)][self.GetTerminalId(CurrentIn)]

                    # Error Handling 
                    if Id == -1: 
                        
                        print("1)Error in input!")
                        print(Tokens)
                        print(Stack)
                        print("Top: ", Top)
                        print("CurrentIn: ", CurrentIn)
                        print("\n\n")
                        exit()

                    Stack.pop()
                    

                    Rhs = self.RhsList[Id]
                    if Rhs != ["eps"]:
                        for Symbol in reversed(Rhs):
                            Stack.append(Symbol)

                
            elif self.IsSemanticRule(Top):
                if Top == "@PUSH":
                    Stack.pop()
                    Top = GetTop(Stack)
                    if Top == CurrentIn:
                        MatchedStack.append(Top)
                        Tokens, CurrentIn = Read(Tokens)
                    else: 
                        print("2)Error in input!")
                        exit()   
                elif Top == "@JZ":
                    print("JZ:")
                    print(Stack)
                    print(Tokens)
                    print("\n\n")
                    y = input()

                elif Top == "@JZCMPL":
                    print("JZCOMPELETED:")
                    print(Stack)
                    print(Tokens)
                    print("\n\n")
                    y = input()
                    

                   
                
                else:
                    Op0 = MatchedStack.pop()
                    if Top == "@PRINT":
                        print(Top,"\t", Op0 )
                    elif Top == "@MOV":
                        Op1 = MatchedStack.pop()
                        print(Top,"\t", Op1, ", ", Op0 )
                    else:
                        pass
                        Op1 = MatchedStack.pop()
                        MatchedStack.append("t" + str(TempCounter))
                        print(Top, "\t", "t"+ str(TempCounter), ", ", Op1, ",", Op0 )
                        TempCounter += 1

                Stack.pop()

            else: # Terminal 
                
                CurrentIn = Tokens[0]
                if (len(Tokens) > 1):
                    Tokens = Tokens[1:]
                Stack.pop()

        return MatchedStack

    
                 

            
    def ReadGrammar(self):
        Flag = 1
        Counter = -1
        for Line in self.GrammarFile:
            Counter += 1
            Line = Line.strip()
            if Line == "" or Line[0] == "#":
                continue
            elif Line[0] == ".":
                L = Line.split("->")
                Elements = L[1].split(" ")
                if L[0][1:] == "OperatorsTwoOperand":
                    self.OperatorsTwoOperand += Elements
                    continue
                elif L[0][1:] == "OperatorsOneOperand":
                    self.OperatorsOneOperand += Elements
                    continue
                elif L[0][1:] == "SemantiRules":
                    self.SemantiRulesList += Elements
                    continue
                elif L[0][1:] == "Registers":
                    self.RegistersList += Elements
                    continue
                elif L[0][1:] == "PseudoRegisters":
                    self.PseudoRegistersList += Elements
                    continue
                elif L[0][1:] == "ScriptVariableType":
                    self.VariableTypeList += Elements
                    continue
                elif L[0][1:] == "AssignmentOperator":
                    self.AssignmentOperator += Elements
                    continue

                self.FunctionsDict[L[0]] = Elements
                continue

            L = Line.split("->")
            Lhs = L[0]
            Rhs = L[1].split(" ")

            HasMapKeyword = False
            MapKeywordIdx1 = 0
            MapKeywordIdx2 = 0
            Idx = 0
            for X in Rhs:
                if X[0] == ".":
                    HasMapKeyword = True
                    MapKeywordIdx1 = Idx 
                elif X[0] == "@":
                    if X[1] == ".":
                        MapKeywordIdx2 = Idx
                   

                Idx += 1

            if not HasMapKeyword:
                self.NonTerminalSet.add(Lhs)
                self.LhsList.append(Lhs)
                self.RhsList.append(Rhs)
                for X in Rhs:
                    if not self.IsNoneTerminal(X) and not self.IsSemanticRule(X) and not X=="eps":
                        self.TerminalSet.add(X)
                    if self.IsSemanticRule(X):
                        pass
            
            else:
             
                for value in self.FunctionsDict[Rhs[MapKeywordIdx1]]:
                    RhsTemp =list(Rhs)
                    RhsTemp[MapKeywordIdx1] = value
                    RhsTemp[MapKeywordIdx2] = "@" + value.upper()

                    self.keywordList.append(value)

                    self.NonTerminalSet.add(Lhs)
                    self.LhsList.append(Lhs)
                    self.RhsList.append(RhsTemp)

                    for X in RhsTemp:
                        if not self.IsNoneTerminal(X) and not self.IsSemanticRule(X) and not X=="eps":
                            self.TerminalSet.add(X)
                
            if Flag:
                Flag = 0
                self.Start = Lhs
            self.MAXIMUM_RHS_LEN = max(self.MAXIMUM_RHS_LEN, len(Rhs))

           

        self.TerminalSet.add("$")
        
        self.NonTerminalList = list(self.NonTerminalSet)
        
        self.TerminalList = list(self.TerminalSet)

        
    def WriteSemanticMaps(self):
        
        self.CommonHeaderFileScala.write("object ScriptEvalFunc {\n  object ScriptOperators extends ChiselEnum {\n    val ")
        
        Counter = 0
        CheckForDuplicateList = []
        
        self.CommonHeaderFile.write("#define " + "FUNC_UNDEFINED " + str(Counter) + "\n")
        self.CommonHeaderFileScala.write("sFunc" + "Undefined")
        Counter += 1
        
        for X in self.OperatorsOneOperand:
        
            if X not in CheckForDuplicateList:
                self.CommonHeaderFile.write("#define " + "FUNC_" + X.upper() + " " + str(Counter) + "\n")
                self.CommonHeaderFileScala.write(", sFunc" + X.capitalize())
                    
                CheckForDuplicateList.append(X)
                Counter += 1

        for X in self.OperatorsTwoOperand:
        
            if X not in CheckForDuplicateList:
                self.CommonHeaderFile.write("#define " + "FUNC_" + X.upper() + " " + str(Counter) + "\n")
                self.CommonHeaderFileScala.write(", sFunc" + X.capitalize())
                CheckForDuplicateList.append(X)
                Counter += 1
        
        for X in self.SemantiRulesList:
        
            if X not in CheckForDuplicateList:
                self.CommonHeaderFile.write("#define " + "FUNC_" + X.upper() + " " + str(Counter) + "\n")
                self.CommonHeaderFileScala.write(", sFunc" + X.capitalize())
                CheckForDuplicateList.append(X)
                Counter += 1
        
        for X in self.keywordList:
        
            if X not in CheckForDuplicateList:
                self.CommonHeaderFile.write("#define " + "FUNC_" + X.upper() + " " + str(Counter) + "\n")
                
                #
                # Check if it's the last item
                #
                self.CommonHeaderFileScala.write(", sFunc" + X.capitalize() + "")
                    
                CheckForDuplicateList.append(X)
                Counter += 1
                
            
        self.CommonHeaderFileScala.write(" = Value\n  }\n} ")


        self.SourceFile.write("const SYMBOL_MAP SemanticRulesMapList[]= {\n")
        self.HeaderFile.write("extern const SYMBOL_MAP SemanticRulesMapList[];\n")

        for X in self.OperatorsOneOperand:
            self.SourceFile.write("{\"@" + X.upper() + "\", "+ "FUNC_" + X.upper()   + "},\n")

        for X in self.OperatorsTwoOperand:
            self.SourceFile.write("{\"@" + X.upper() + "\", "+ "FUNC_" + X.upper()   + "},\n")

        for X in self.SemantiRulesList:
            self.SourceFile.write("{\"@" + X.upper() + "\", "+ "FUNC_" + X.upper()   + "},\n")

        for X in self.keywordList:
                self.SourceFile.write("{\"@" + X.upper() + "\", "+ "FUNC_" + X.upper()   + "},\n")

        for X in self.AssignmentOperator:
            self.SourceFile.write("{\"@" + X.upper() + "\", "+ "FUNC_" + X.upper().replace("_ASSIGNMENT", "")   + "},\n")

        self.SourceFile.write("};\n")

        CheckForDuplicateList = []
        self.CommonHeaderFile.write("\nstatic const char *const FunctionNames[] = {")
        self.CommonHeaderFile.write("\n\"FUNC_UNDEFINED\""+ ",\n")
        for X in self.OperatorsOneOperand:
            if X not in CheckForDuplicateList:
                self.CommonHeaderFile.write("\"" + "FUNC_" + X.upper() + "\"" + ",\n")
                CheckForDuplicateList.append(X)

        for X in self.OperatorsTwoOperand:
            if X not in CheckForDuplicateList:
                self.CommonHeaderFile.write("\"" + "FUNC_" + X.upper() + "\"" + ",\n")
                CheckForDuplicateList.append(X)
    
        for X in self.SemantiRulesList:
            if X not in CheckForDuplicateList:
                self.CommonHeaderFile.write("\"" + "FUNC_" + X.upper() + "\"" + ",\n")
                CheckForDuplicateList.append(X)
        
        for X in self.keywordList:
            if X not in CheckForDuplicateList:
                self.CommonHeaderFile.write("\"" + "FUNC_" + X.upper() + "\"" + ",\n")
                CheckForDuplicateList.append(X)

        self.CommonHeaderFile.write("};\n")


    def WriteRegisterMaps(self):
        self.CommonHeaderFile.write("\ntypedef enum REGS_ENUM {\n")
        Counter = 0          
        for X in self.RegistersList:
            if Counter == len(self.RegistersList)-1:
                self.CommonHeaderFile.write("\t" + "REGISTER_" + X.upper() + " = " + str(Counter) + "\n")
            else:
                self.CommonHeaderFile.write("\t" + "REGISTER_" + X.upper() + " = " + str(Counter) + ",\n")
            Counter += 1
        self.CommonHeaderFile.write("\n} REGS_ENUM;\n\n")

        self.CommonHeaderFile.write("static const char *const RegistersNames[] = {\n")
        Counter = 0 
        for X in self.RegistersList:
            if Counter == len(self.RegistersList)-1:
                self.CommonHeaderFile.write("\t" + "\"" + X + "\"")
            else:
                if (Counter + 1) % 8 == 0:
                    self.CommonHeaderFile.write("" + "\"" + X + "\",\n")
                else:
                    self.CommonHeaderFile.write("" + "\"" + X + "\", ")

            Counter += 1
        self.CommonHeaderFile.write("\n};\n\n")




        self.SourceFile.write("const SYMBOL_MAP RegisterMapList[]= {\n")
        self.HeaderFile.write("extern const SYMBOL_MAP RegisterMapList[];\n")

        Counter = 0
        for X in self.RegistersList:
            if Counter == len(self.RegistersList)-1:
                self.SourceFile.write("{\"" + X + "\", "+ "REGISTER_" + X.upper()   + "}\n")
            else:
                self.SourceFile.write("{\"" + X + "\", "+ "REGISTER_" + X.upper()   + "},\n")
            Counter +=1
        self.SourceFile.write("};\n")


    def WritePseudoRegMaps(self):
        Counter = 0          
        for X in self.PseudoRegistersList:
            self.CommonHeaderFile.write("#define " + "PSEUDO_REGISTER_" + X.upper() + " " + str(Counter) + "\n")
            Counter += 1
        self.CommonHeaderFile.write("\n")

        self.SourceFile.write("const SYMBOL_MAP PseudoRegisterMapList[]= {\n")
        self.HeaderFile.write("extern const SYMBOL_MAP PseudoRegisterMapList[];\n")

        Counter = 0
        for X in self.PseudoRegistersList:
            if Counter == len(self.PseudoRegistersList)-1:
                self.SourceFile.write("{\"" + X + "\", "+ "PSEUDO_REGISTER_" + X.upper()   + "}\n")
            else:
                self.SourceFile.write("{\"" + X + "\", "+ "PSEUDO_REGISTER_" + X.upper()   + "},\n")
            Counter +=1
        self.SourceFile.write("};\n")

    def WriteVariableTypeList(self):
        self.SourceFile.write("const char* ScriptVariableTypeList[]= {\n")
        self.HeaderFile.write("extern const char* ScriptVariableTypeList[];\n")

        Counter = 0
        for X in self.VariableTypeList:
            if Counter == len(self.VariableTypeList)-1:
                self.SourceFile.write("\"" + X + "\""   + "\n")
            else:
                self.SourceFile.write("\"" + X + "\""  + ",\n")
            Counter +=1
        self.SourceFile.write("};\n")

    def WriteKeywordList(self):
        self.SourceFile.write("const char* KeywordList[]= {\n")
        self.HeaderFile.write("extern const char* KeywordList[];\n")

        Counter = 0
        for X in self.keywordList:
            if Counter == len(self.keywordList)-1:
                self.SourceFile.write("\"" + X + "\"" + "\n")
            else:
                self.SourceFile.write("\"" + X + "\"" + ",\n")
            Counter +=1
        self.SourceFile.write("};\n")

    def WriteOperatorsList(self):
        self.SourceFile.write("const char* OperatorsTwoOperandList[]= {\n")
        self.HeaderFile.write("extern const char* OperatorsTwoOperandList[];\n")

        Counter = 0
        for X in self.OperatorsTwoOperand:
            if Counter == len(self.OperatorsTwoOperand)-1:
                self.SourceFile.write("\"" + "@"+ X.upper() + "\"" + "\n")
            else:
                self.SourceFile.write("\"" + "@"+ X.upper() + "\"" + ",\n")
            Counter +=1
        self.SourceFile.write("};\n")

        self.SourceFile.write("const char* OperatorsOneOperandList[]= {\n")
        self.HeaderFile.write("extern const char* OperatorsOneOperandList[];\n")

        Counter = 0
        for X in self.OperatorsOneOperand:
            if Counter == len(self.OperatorsOneOperand)-1:
                self.SourceFile.write("\"" + "@"+ X.upper() + "\"" + "\n")
            else:
                self.SourceFile.write("\"" + "@"+ X.upper() + "\"" + ",\n")
            Counter +=1
        self.SourceFile.write("};\n")

        self.SourceFile.write("const char* AssignmentOperatorList[]= {\n")
        self.HeaderFile.write("extern const char* AssignmentOperatorList[];\n")

        Counter = 0
        for X in self.AssignmentOperator:
            if Counter == len(self.AssignmentOperator)-1:
                self.SourceFile.write("\"" + "@"+ X.upper() + "\"" + "\n")
            else:
                self.SourceFile.write("\"" + "@"+ X.upper() + "\"" + ",\n")
            Counter +=1
        self.SourceFile.write("};\n")        

     

    def WriteMaps(self):
        for Key in self.FunctionsDict:
            print(Key)

            self.HeaderFile.write("extern const char* "+ Key[1:]+ "[];\n")
            self.SourceFile.write("const char* "+ Key[1:]+ "[] = {\n")

            Counter = 0
            for X in self.FunctionsDict[Key]:
                if Counter == len(self.FunctionsDict[Key])-1:
                    self.SourceFile.write("\"" + "@"+ X.upper() + "\"" + "\n")
                else:
                    self.SourceFile.write("\"" +"@"+ X.upper() + "\"" + ",\n")
            Counter +=1
            self.SourceFile.write("};\n")



    def WriteLhsList(self):

        self.SourceFile.write("const struct _TOKEN Lhs[RULES_COUNT]= \n{\n")
        self.HeaderFile.write("extern const struct _TOKEN Lhs[RULES_COUNT];\n")
        Counter = 0
        for Lhs in self.LhsList:
            if Counter == len(self.LhsList)-1:
                self.SourceFile.write("\t{NON_TERMINAL, " + "\"" + Lhs + "\"}" + "\n")
            else:
                self.SourceFile.write("\t{NON_TERMINAL, " + "\"" + Lhs + "\"}" + ",\n")
            Counter +=1
        self.SourceFile.write("};\n")

    def GetType(self,Var):
        if self.IsNoneTerminal(Var):
            return "NON_TERMINAL"
        elif self.IsSemanticRule(Var):
            return "SEMANTIC_RULE"

        elif Var == "eps":
            return "EPSILON"

        elif Var in self.SPECIAL_TOKENS:
            return "SPECIAL_TOKEN"
        elif Var[0] == "_":
            return Var[1:].upper()
        else:
            return "KEYWORD"


    def WriteRhsList(self):
        self.SourceFile.write("const struct _TOKEN Rhs[RULES_COUNT][MAX_RHS_LEN]= \n{\n")
        self.HeaderFile.write("extern const struct _TOKEN Rhs[RULES_COUNT][MAX_RHS_LEN];\n")
        Counter =0
        for Rhs in self.RhsList:
            self.SourceFile.write("\t{")

            C = 0
            for Var in Rhs:
                if C == len(Rhs) -1:
                    self.SourceFile.write("{"+self.GetType(Var) +", "+"\"" + Var + "\"}" )
                else:
                    self.SourceFile.write("{"+self.GetType(Var) +", "+"\"" + Var + "\"}," )
                C += 1

            if Counter == len(self.RhsList)-1:
                self.SourceFile.write("}\n")
            else:
                self.SourceFile.write("},\n")
            Counter+= 1

        self.SourceFile.write("};\n")

    def WriteRhsSize(self):
        self.SourceFile.write("const unsigned int RhsSize[RULES_COUNT]= \n{\n")
        self.HeaderFile.write("extern const unsigned int RhsSize[RULES_COUNT];\n")
        Counter =0
        for Rhs in self.RhsList:            
            if Counter == len(self.RhsList)-1:
               self.SourceFile.write( str(len(Rhs)) + "\n" )
            else:
                self.SourceFile.write( str(len(Rhs)) + ",\n" )
            Counter+= 1

        self.SourceFile.write("};\n")

    def WriteTerminalList(self):
        self.SourceFile.write("const char* TerminalMap[TERMINAL_COUNT]= \n{\n")
        self.HeaderFile.write("extern const char* TerminalMap[TERMINAL_COUNT];\n")
        Counter = 0
        for X in self.TerminalList:
            if Counter == len(self.TerminalList)-1:
                self.SourceFile.write("\"" + X + "\"" + "\n")
            else:
                self.SourceFile.write("\"" + X + "\"" + ",\n")
            Counter +=1
        self.SourceFile.write("};\n")

    def WriteNoneTermianlList(self):
        self.SourceFile.write("const char* NoneTerminalMap[NONETERMINAL_COUNT]= \n{\n")
        self.HeaderFile.write("extern const char* NoneTerminalMap[NONETERMINAL_COUNT];\n")
        Counter = 0
        for X in self.NonTerminalList:
            if Counter == len(self.NonTerminalList)-1:
                self.SourceFile.write("\"" + X + "\"" + "\n")
            else:
                self.SourceFile.write("\"" + X + "\"" + ",\n")
            Counter +=1
        self.SourceFile.write("};\n")

    def WriteParseTable(self):
        self.SourceFile.write("const int ParseTable[NONETERMINAL_COUNT][TERMINAL_COUNT]= \n{\n")
        self.HeaderFile.write("extern const int ParseTable[NONETERMINAL_COUNT][TERMINAL_COUNT];\n")
        i = 0
        for X in self.NonTerminalList:
            j = 0
            self.SourceFile.write("\t{")
            for y in self.TerminalList:
                self.SourceFile.write(str(self.ParseTable[i][j]))
                if j != len(self.TerminalList)-1:
                    self.SourceFile.write("\t\t,")
                j += 1

            if i == len(self.NonTerminalList)-1:
                self.SourceFile.write("\t}\n")

            else:
                self.SourceFile.write("\t},\n")
            i +=1
        self.SourceFile.write("};\n")
        


    def FindAllFirsts(self):
        
        self.FirstDict = {}
        for Symbol in self.NonTerminalList:
            self.FirstDict[Symbol] = set()

        t = 0 
        while True:
            Updated = False 
            i = 0 
            for Lhs in self.LhsList:
                Rhs = self.RhsList[i]
                Temp = set(self.FirstDict[Lhs])

                
                if Rhs[0] == "eps":
                    pass
                elif self.IsNoneTerminal(Rhs[0]):
                    self.FirstDict[Lhs] = self.FirstDict[Lhs].union(self.FirstDict[Rhs[0]])
                    p = 0
                    while self.IsNullable(Rhs[p]):
                        self.FirstDict[Lhs] = self.FirstDict[Lhs].union(self.FirstDict[Rhs[p+1]])
                        p += 1
                elif self.IsSemanticRule(Rhs[0]):
                    if self.IsNoneTerminal(Rhs[0]):
                        self.FirstDict[Lhs] = self.FirstDict[Lhs].union(self.FirstDict[Rhs[1]])
                    else: 
                        self.FirstDict[Lhs].add(Rhs[1])
                else:
                    self.FirstDict[Lhs].add(Rhs[0])
                i += 1
               
                if Temp != self.FirstDict[Lhs]:
                    Updated = True
            p += 1  
            if not Updated:
                break;    
            


    def PrintFirsts(self):
        for Id in self.FirstDict:
            print(Id, end=": ")
            for X in self.FirstDict[Id]:
                print(X,end = " ")
            print()

    def GetNoneTerminalId(self, nonterminal):
        for i in range(len(self.NonTerminalList)):
            if nonterminal == self.NonTerminalList[i]:
                return i
        return -1

    def GetTerminalId(self, Terminal):
        for i in range(len(self.TerminalList)):
            if Terminal == self.TerminalList[i]:
                return i
        return -1


    def FillParseTable(self):
        self.ParseTable = [[self.INVALID for y in range(len(self.TerminalList))] for X in range(len(self.NonTerminalList))]

        RuleId = 0 
        for Lhs in self.LhsList:
            i = self.GetNoneTerminalId(Lhs)

            j = 0
            for Terminal in self.TerminalList:
                if Terminal in self.PredictDict[RuleId]:
                    if i ==34 and j ==3:
                        dccc=3

                    if self.ParseTable[i][j] == self.INVALID:
                        self.ParseTable[i][j] = RuleId

                    else:

                        print("Error! Input grammar is not LL1.")
                        exit()

                j += 1

            RuleId += 1

    def PrintParseTable(self):
        print("\t", end = "")
        for j in range(len(self.TerminalList)):
            print(self.TerminalList[j], end= "\t")
        print()
        for i in range(len(self.NonTerminalList)):
            print(self.NonTerminalList[i], end= "\t")
            for j in range(len(self.TerminalList)):
                if self.ParseTable[i][j] == self.INVALID:
                    print(".", end= "\t")
                else:
                    print(self.ParseTable[i][j], end= "\t")
            print()
                

    def FindAllPredicts(self):
        self.PredictDict = {}
        for i in range(len(self.LhsList)):
            self.PredictDict[i] = set()

        i = 0
        for Lhs in self.LhsList:
            Rhs = self.RhsList[i]
            IsRightNullable = True
            for Symbol in Rhs:
                if self.IsSemanticRule(Symbol):
                    pass
                
                elif Symbol == "eps":
                    IsRightNullable = True
                    break
                elif self.IsNoneTerminal(Symbol):
                    self.PredictDict[i] |= self.FirstDict[Symbol]
                    if not self.IsNullable(Symbol):
                        IsRightNullable = False
                        break
                else:
                    self.PredictDict[i].add(Symbol)
                    IsRightNullable = False
                    break
            
            if IsRightNullable:
                self.PredictDict[i] |= self.FollowDict[Lhs]
            i += 1
               

    def print_predicts(self):
        for Key in self.PredictDict:
            print(Key, end=": ")
            for X in self.PredictDict[Key]:
                print(X,end = " ")
            print()


    
    def FindAllFollows(self):
        self.FollowDict = {}
        for Symbol in self.NonTerminalList:
            self.FollowDict[Symbol] = set()
            if Symbol == self.Start:
                self.FollowDict[Symbol].add('$')

        t = 0
        while True: 
            Updated = False 

            i = 0
            for Lhs in self.LhsList:
                Rhs = self.RhsList[i]

                p = 0
                for Symbol in Rhs:
                    
                    if self.IsNoneTerminal(Symbol):
                        
                        Temp = set(self.FollowDict[Symbol])
                        if p == len(Rhs)-1:
                            self.FollowDict[Symbol] = self.FollowDict[Symbol].union(self.FollowDict[Lhs])
                        
                        
                        
                        else:

                            NextVar = self.GetNextVar(Rhs, p)
                            if self.IsNoneTerminal(NextVar):
                                self.FollowDict[Symbol] = self.FollowDict[Symbol].union(self.FirstDict[NextVar])
                                if self.IsNullable(NextVar):    
                                    self.FollowDict[Symbol] = self.FollowDict[Symbol].union(self.FollowDict[NextVar])                                    
                            elif self.IsSemanticRule(NextVar):
                                pass      
                            else:
                                self.FollowDict[Symbol].add(NextVar)
                        
                        if p == len(Rhs)-2:
                            NextVar = self.GetNextVar(Rhs, p)
                            
                            if self.IsNullable(NextVar):
                                self.FollowDict[Symbol] = self.FollowDict[Symbol].union(self.FollowDict[Lhs])
                                self.FollowDict[Lhs] = self.FollowDict[Lhs].union(self.FollowDict[NextVar])
                        
                   

                        if Temp != self.FollowDict[Symbol]:
                            Updated = True
                    p += 1  
                i += 1
            t += 1

            if not Updated: 
                break
    def GetNextVar(self, Rhs, p):
        if p == len(Rhs)-1:
            return None
        
        X = p +1
        while self.IsSemanticRule(Rhs[X]) and X + 1 < len(Rhs):
            X+=1
        return Rhs[X]
        
    def PrintFollows(self):
        for Key in self.FollowDict:
            print(Key, end=": ")
            for X in self.FollowDict[Key]:
                print(X,end = " ")
            print()

    def IsNoneTerminal(self,X):     
        if X[0].isupper():
            return True
        else:
            return False

    def IsSemanticRule(self, X):
        if X[0] == '@':
            return True
        else: 
            return False

    def IsNullable(self, s):

        if not s[0].isupper():
            return False
        i = 0 
        for Lhs in self.LhsList:
            Rhs = self.RhsList[i]
            
            if Lhs == s:

                if Rhs[0] == "eps":
                    return True

                p = 0 
                while True:
                    if self.IsNullable(Rhs[p]):
                        return True
                    else:
                        break
                    p += 1
            i +=1
                    
        return False
