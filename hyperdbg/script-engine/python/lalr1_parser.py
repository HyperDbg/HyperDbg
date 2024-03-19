"""
 * @file lalr1_parse_table_generator.py
 * @author M.H. Gholamrezei (mh@hyperdbg.org)
 * @brief Script engine LALR(1) Parse table generator 
 * @details This program reads grammar from Boolean_Expression_Grammar.txt file 
 *          placed in the same directory of the program 
 *          and creates parse_table.h and parse_table.c which is 
 *          used by the parser of script engine. 
 * @version 0.1
 * @date 2021-02-07
 *
 * @copyright This project is released under the GNU Public License v3.
 
 """

from lalr_parsing import *
from lalr_parsing.grammar import *
from util import *
from ll1_parser import *

class LALR1Parser:
    def __init__(self, SourceFile, HeaderFile):
        # The file which contains the grammar of the language 
        self.GrammarFile = open("Boolean_Expression_Grammar.txt", "r")
        self.SourceFile = SourceFile
        self.HeaderFile = HeaderFile    

        # Lists which used for storing the rules:
        # Right Hand Side(Rhs)
        self.RhsList = []

        self.SemanticList = []

        # Left Hand Side(Lhs)
        self.LhsList = []

        # Set of all terminals and noneterminals
        self.TerminalSet = set()
        self.NonTerminalSet = set()

        # Start variable 
        self.Start = ""

        # maximum of "Right Hand Side(Rhs)" length
        self.MAXIMUM_RHS_LEN = 0

        self.SPECIAL_TOKENS = ['%', '+', '++', '-', '--', "*", "/", "=", "==", "!=", ",", ";", "(", ")", "{", "}", "|", "||", ">>", ">=", "<<", "<=", "&", "&&", "^"]



        # INVALID rule indicator
        self.INVALID = 0x80000000 
        self.ACCEPT = 0x7fffffff

        self.FunctionsDict = dict()
        self.OperatorsList = []
        self.RegistersList = []
        self.PseudoRegistersList = []
        self.keywordList = []
        self.ParseTable = None 
    
    def Run(self):
        self.StateCount = len(list(self.ParseTable.goto))

        # Prints variables that are needed for parser for parsing into the output file 
        self.HeaderFile.write("#define LALR_RULES_COUNT " + str(len(self.LhsList)) + "\n")
        self.HeaderFile.write("#define LALR_TERMINAL_COUNT " + str(len(list(self.TerminalSet))) + "\n")
        self.HeaderFile.write("#define LALR_NONTERMINAL_COUNT " + str(len(list(self.NonTerminalList))) + "\n")
        self.HeaderFile.write("#define LALR_MAX_RHS_LEN "  + str(self.MAXIMUM_RHS_LEN) +"\n")
        self.HeaderFile.write("#define LALR_STATE_COUNT "  + str(self.StateCount) +"\n")

        
        
       

        
        #  Prints Rules into output files
        self.WriteLhsList()
        self.WriteRhsList()
        
        # Prints size of each Rhs into output files
        self.WriteRhsSize()

        # Prints noneterminals and Terminal into output files
        self.WriteNoneTermianlList()
        self.WriteTerminalList()

        self.WriteParseTable()
        self.WriteSemanticRules()

        self.HeaderFile.write("#endif\n")
        
        

    def WriteLhsList(self):

        self.SourceFile.write("const struct _TOKEN LalrLhs[RULES_COUNT]= \n{\n")
        self.HeaderFile.write("extern const struct _TOKEN LalrLhs[RULES_COUNT];\n")
        Counter = 0
        for Lhs in self.LhsList:
            if Counter == len(self.LhsList)-1:
                self.SourceFile.write("\t{NON_TERMINAL, " + "\"" + Lhs + "\"}" + "\n")
            else:
                self.SourceFile.write("\t{NON_TERMINAL, " + "\"" + Lhs + "\"}" + ",\n")
            Counter +=1
        self.SourceFile.write("};\n")

    def WriteRhsList(self):
        self.SourceFile.write("const struct _TOKEN LalrRhs[RULES_COUNT][MAX_RHS_LEN]= \n{\n")
        self.HeaderFile.write("extern const struct _TOKEN LalrRhs[RULES_COUNT][MAX_RHS_LEN];\n")
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
        self.SourceFile.write("const unsigned int LalrRhsSize[RULES_COUNT]= \n{\n")
        self.HeaderFile.write("extern const unsigned int LalrRhsSize[RULES_COUNT];\n")
        Counter =0
        for Rhs in self.RhsList:            
            if Counter == len(self.RhsList)-1:
               self.SourceFile.write( str(len(Rhs)) + "\n" )
            else:
                self.SourceFile.write( str(len(Rhs)) + ",\n" )
            Counter+= 1

        self.SourceFile.write("};\n")

    def WriteTerminalList(self):
        self.SourceFile.write("const char* LalrTerminalMap[TERMINAL_COUNT]= \n{\n")
        self.HeaderFile.write("extern const char* LalrTerminalMap[TERMINAL_COUNT];\n")
        Counter = 0
        for X in self.TerminalList:
            if Counter == len(self.TerminalList)-1:
                self.SourceFile.write("\"" + X + "\"" + "\n")
            else:
                self.SourceFile.write("\"" + X + "\"" + ",\n")
            Counter +=1
        self.SourceFile.write("};\n")

    def WriteNoneTermianlList(self):
        self.SourceFile.write("const char* LalrNoneTerminalMap[NONETERMINAL_COUNT]= \n{\n")
        self.HeaderFile.write("extern const char* LalrNoneTerminalMap[NONETERMINAL_COUNT];\n")
        Counter = 0
        for X in self.NonTerminalList:
            if Counter == len(self.NonTerminalList)-1:
                self.SourceFile.write("\"" + X + "\"" + "\n")
            else:
                self.SourceFile.write("\"" + X + "\"" + ",\n")
            Counter +=1
        self.SourceFile.write("};\n")

 
    def WriteSemanticRules(self):
        
        self.SourceFile.write("const struct _TOKEN LalrSemanticRules[RULES_COUNT]= \n{\n")
        self.HeaderFile.write("extern const struct _TOKEN LalrSemanticRules[RULES_COUNT];\n") 

        Counter = 0 
        for SemanticRule in self.SemanticList:
            if Counter == len(self.SemanticList)-1:
                if SemanticRule == None:
                    self.SourceFile.write("\t{UNKNOWN, " + "\""+"\"}" + "\n")
                else:
                    self.SourceFile.write("\t{SEMANTIC_RULE, " + "\"" + SemanticRule+ "\"}" + "\n")
            else:
                if SemanticRule == None:
                    self.SourceFile.write("\t{UNKNOWN, " + "\""+"\"}" + ",\n")
                else:
                    self.SourceFile.write("\t{SEMANTIC_RULE, " + "\"" + SemanticRule+ "\"}" + ",\n")
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
                if L[0][1:] == "Operators":
                    self.OperatorsList += Elements
                    continue
                elif L[0][1:] == "Registers":
                    self.RegistersList += Elements
                    continue
                elif L[0][1:] == "PseudoRegisters":
                    self.PseudoRegistersList += Elements
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


        ii = 0
        for Rhs in self.RhsList:
            if self.IsSemanticRule(Rhs[len(Rhs)-1]):
                self.SemanticList.append(Rhs[len(Rhs)-1])
            else:
                self.SemanticList.append(None)
            ii += 1

        self.TerminalSet.add("$")
        
        self.NonTerminalList = list(self.NonTerminalSet)
        
        self.TerminalList = list(self.TerminalSet)

    def FillActionTable(self):
        self.ActionTable = [[self.INVALID for y in range(len(self.TerminalList))] for X in range(self.StateCount)]
    
        for RowId in range(self.StateCount):
            
            for Terminal in self.ParseTable.action[RowId].keys():
                Action = self.ParseTable.action[RowId][Terminal] 
                Type = None
                StateId = None
                for Element in Action:
                    Type = Element[0]
                    StateId = Element[1]
                if Type != None:
                    if Type == 'S':
                        self.ActionTable[RowId][self.GetTerminalId(Terminal)] = StateId
                    elif Type == 'R':
                        self.ActionTable[RowId][self.GetTerminalId(Terminal)] = str(-int(StateId))
                    elif Type == 'accept':
                        self.ActionTable[RowId][self.GetTerminalId(Terminal)] = self.ACCEPT
                
        
    def FillGotoTable(self):
        self.GotoTable = [[self.INVALID for y in range(len(self.NonTerminalList))] for X in range(self.StateCount)]
        for RowId in range(self.StateCount):
            
            for NonTerminal in self.ParseTable.goto[RowId].keys():
                Goto = self.ParseTable.goto[RowId][NonTerminal] 
                if Goto != None:
                    Id = self.GetNoneTerminalId(NonTerminal.name)
                    self.GotoTable[RowId][Id] = Goto
        
            

    def WriteParseTable(self):
        # Action = self.ParseTable.action[Top][CurrentIn] 
      
        self.FillGotoTable()
        self.WriteGotoTable()

        self.FillActionTable()
        self.WriteActionTable()
        




    def WriteActionTable(self):
        self.SourceFile.write("const int LalrActionTable[LALR_STATE_COUNT][LALR_TERMINAL_COUNT]= \n{\n")
        self.HeaderFile.write("extern const int LalrActionTable[LALR_STATE_COUNT][LALR_TERMINAL_COUNT];\n")
        i = 0
        for Row in self.ActionTable:
            j = 0
            self.SourceFile.write("\t{")   
            for Element in Row:
                self.SourceFile.write(str(self.ActionTable[i][j]))
                if j != len(Row)-1:
                    self.SourceFile.write("\t\t,")
                j += 1
            if i == len(self.ActionTable)-1:
                self.SourceFile.write("\t}\n")

            else:
                self.SourceFile.write("\t},\n")
            i +=1
        self.SourceFile.write("};\n")
        
    def WriteGotoTable(self):
        self.SourceFile.write("const int LalrGotoTable[LALR_STATE_COUNT][LALR_NONTERMINAL_COUNT]= \n{\n")
        self.HeaderFile.write("extern const int LalrGotoTable[LALR_STATE_COUNT][LALR_NONTERMINAL_COUNT];\n")
        i = 0
        for Row in self.GotoTable:
            self.SourceFile.write("\t{")   
            for j in range(len(Row)):
                self.SourceFile.write(str(self.GotoTable[i][j]))
                if j != len(Row)-1:
                    self.SourceFile.write("\t\t,")
                
            if i == self.StateCount-1:
                self.SourceFile.write("\t}\n")

            else:
                self.SourceFile.write("\t},\n")
            i +=1
        self.SourceFile.write("};\n")



        
    
    # This function simulates of script engine parser in ScriptEngine.c in
    # order to test the generated "Parse Table"
    def Parse(self, Tokens, InputSize):
        # print("Lalr parser called...\n")
        # print("Tokens : ", end = "")
        # print(Tokens)

        ReadCount = 0

        Stack = [ 0 ]
        MatchedStack = []
        
        if ReadCount < InputSize:
            Tokens, CurrentIn = Read(Tokens)
            CurrentIn = "'" + CurrentIn + "'"
            ReadCount +=1
        else:
            CurrentIn = "'$end'"

        # print("OpenParanthesisCount = ", OpenParenthesesCount)
      
        
        while True:
            # Read top of stack 
            Top = GetTop(Stack)
            # for key, value in self.ParseTable.action.items() :
            #     print (key, value)
            # print("CurrentIn: ", CurrentIn)
            Action = self.ParseTable.action[Top][CurrentIn] 
            # print("Action:",Action)
            # print()
            Type = None
            StateId = None
            for Element in Action:
                Type = Element[0]
                StateId = Element[1]
            # print("Stack:", end =" ")
            # print(Stack)

            # print("Tokens:", end =" ")
            # print(Tokens)
            
            # print("CurrentIn: ", CurrentIn)
            
            # print("Action : ", Action,'\n\n')
            
            # print("Matched Stack:", end = " ")
            # print(MatchedStack)
        

            
            
            if Type == 'S':
                Stack.append(CurrentIn)
                Stack.append(StateId)

                if ReadCount < InputSize:
                    Tokens, CurrentIn = Read(Tokens)
                    CurrentIn = "'" + CurrentIn + "'"
                    ReadCount += 1
                else:
                    CurrentIn = "'$end'"
                # print("CurrentIn = ", CurrentIn)


                # print("Stack:", end =" ")
                # print(Stack)

                # print("Tokens:", end =" ")
                # print(Tokens)
                
                # print("CurrentIn: ", CurrentIn)
                
                # print("Action : ", Action,'\n\n')

                # print("OpenParanthesisCount = ", OpenParenthesesCount, end = "\n\n")

                # if OpenParenthesesCount == 0: 
                #     print("1) LALR Returned...\n")
                #     return Tokens

                # print("Shift ", StateId, ": ")
                # print(Stack) 
                # print()
                # x = input()

            elif Type == 'R':

                def get_length(Rhs):
                    i = 0 
                    for X in Rhs:
                        if self.IsSemanticRule(X):
                            continue
                        elif X == 'eps':
                            continue
                        i += 1
                    return i
                

                
               

                Lhs = self.LhsList[StateId - 1]
                Rhs = self.RhsList[StateId - 1]
                RhsLen = get_length(Rhs)

                SemanticRule = self.SemanticList[StateId - 1]
                # print("Semantic Rule :", SemanticRule)
                # print("Rhs : ", Rhs)
                Operand = None 
                for i in range(RhsLen * 2):
                    Temp = Stack.pop()
                    # print("Temp : ", Temp)
                    if type(Temp) == str:
                        print("Temp : ", Temp)
                        Operand = Temp
                if SemanticRule == "@PUSH":
                    MatchedStack.append(Operand)
                if SemanticRule == "@GT":
                    print("Matched Stack:", end = " ")
                    print(MatchedStack)

                    Operand0 = MatchedStack.pop()
                    Operand1 = MatchedStack.pop()
                    print("GT ", Operand0, " ", Operand1)   
                    MatchedStack.append(Operand0)    
                
                if SemanticRule == "@LT":
                    print("Matched Stack:", end = " ")
                    print(MatchedStack)

                    Operand0 = MatchedStack.pop()
                    Operand1 = MatchedStack.pop()
                    print("LT ", Operand0, " ", Operand1)   
                    MatchedStack.append(Operand0)

                elif SemanticRule == "@AND":
                    print("Matched Stack:", end = " ")
                    print(MatchedStack)

                    Operand0 = MatchedStack.pop()
                    Operand1 = MatchedStack.pop()
                    print("AND ", Operand0, " ", Operand1)   
                    MatchedStack.append(Operand0)    
                # x = input()
                # print("\n\n")

                StateId = GetTop(Stack)

                Top = GetTop(Stack)
                Stack.append(Lhs)


                LhsNonTerm = None                 
                for NonTerm in self.ParseTable.nonterms:
                    if Lhs == NonTerm.name:
                        LhsNonTerm = NonTerm
                # print(LhsNonTerm.name)

                Goto = self.ParseTable.goto[StateId][LhsNonTerm]

                Stack.append(Goto)
                # print("Reduce ", StateId, ": ")
                # print(Action)
                # print(Stack)
                # print()
                # x = input()

            elif Type == 'accept':
                # print("Accepted")
                # print()
                # x = input()
                return Tokens
            else :
                
                # print("Error")
                # print(Stack)
                # print()
                # x = input()
                pass


            
            
                

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

    def GetNoneTerminalId(self, NonTerminal):
        for i in range(len(self.NonTerminalList)):
            if NonTerminal == self.NonTerminalList[i]:
                return i
        return -1

    def GetTerminalId(self, Terminal):
        if Terminal == "'$end'":
            X = "$"
        else:
            X = Terminal[1:-1]
        for i in range(len(self.TerminalList)):
            if X == self.TerminalList[i]:
                return i
        return -1


    
def describe_grammar(gr):
    return '\n'.join([
        'Indexed grammar rules (%d in total):' % len(gr.productions),
        str(gr) + '\n',
        'Grammar non-terminals (%d in total):' % len(gr.nonterms),
        '\n'.join('\t' + str(s) for s in gr.nonterms) + '\n',
        'Grammar terminals (%d in total):' % len(gr.terminals),
        '\n'.join('\t' + str(s) for s in gr.terminals)
    ])


def describe_parsing_table(table):
    conflict_status = table.get_conflict_status()

    def conflict_status_str(state_id):
        has_sr_conflict = (conflict_status[state_id] == lalr_one.STATUS_SR_CONFLICT)
        status_str = ('shift-reduce' if has_sr_conflict else 'reduce-reduce')
        return 'State %d has a %s conflict' % (state_id, status_str)

    return ''.join([
        'PARSING TABLE SUMMARY\n',
        'Is the given grammar LALR(1)? %s\n' % ('Yes' if table.is_lalr_one() else 'No'),
        ''.join(conflict_status_str(sid) + '\n' for sid in range(table.n_states)
                if conflict_status[sid] != lalr_one.STATUS_OK) + '\n',
        table.stringify()
    ])

def get_grammar(parser):
    Rules = []
    i = 0
    preLhs = parser.LhsList[0]
    RhsList = []
    for Lhs in parser.LhsList:
        Rhs = ""

        for X in parser.RhsList[i]:
            if parser.IsNoneTerminal(X):
                Rhs += X
            elif parser.IsSemanticRule(X):
                continue
            else:
                Rhs += ("'" + X + "'")
            Rhs += " "
        if parser.RhsList[i][0] == 'eps':
            Rhs = ""

        if preLhs != Lhs:
            Rules.append(NonTerminal(preLhs, RhsList))
            RhsList = []


        RhsList.append(Rhs)
       
        i = i + 1
        preLhs = Lhs

    Rules.append(NonTerminal(preLhs, RhsList))
    
    return Grammar(Rules)


def main():
    print('Working on it...')
    parser = LALR1Parser()
    parser.Run()
    gr = get_grammar(parser)
    table = lalr_one.ParsingTable(gr)
    print("I'm done.")

    output_filename = 'parsing-table'

    with open(output_filename + '.txt', 'w') as textfile:
        textfile.write(describe_grammar(gr))
        textfile.write('\n\n')
        textfile.write(describe_parsing_table(table))

    table.save_to_csv(output_filename + '.csv')
    parser.Parse(table, ['(', '_hex', '&', 'id', '>', 'id', ')', '&', '(', 'id', '<', 'id',')', '$end'])
