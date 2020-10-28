"""
 * @file parse_table_generator.py
 * @author M.H. Gholamrezei (gholamrezaei.mh@gmail.com)
 * @brief Script engine Parse table generator 
 * @details This program reads grammer from Greammer.txt file 
 *          placed in the same directory of the program and 
 *          and creates ParseTable.h which is used by the 
 *          parser of script engine. 
 * @version 0.1
 * @date 2020-10-24
 *
 * @copyright This project is released under the GNU Public License v3.
 
 """


# Returns Top of the list L 
def GetTop(L):
    return L[len(L)-1]

# Remove first element of list and return it 
def Read(Tokens):
    X = Tokens[0]
    Tokens = Tokens[1:]
    return Tokens, X 

class Parser:
    def __init__(self):
        # The file which contains the grammer of the language 
        self.GrammerFile = open("Grammer.txt", "r")

        # The file which is used by parser for parsing the input 
        self.OutputFile = open("..\src\parse_table.h", "w")

        # Lists which used for storing the rules:
        # Right Hand Side(Rhs)
        self.RhsList = []
        # Left Hand Side(Lhs)
        self.LhsList = []

        # Set of all termials and noneterminals
        self.TerminalSet = set()
        self.NonTerminalSet = set()

        # Start variable 
        self.Start = ""


        # maximum of "Right Hand Side(Rhs)" length
        self.MAXIMUM_RHS_LEN = 0


        self.SPECIAL_TOKENS = ['%', '++', '+=', '+', '--', '-=', '-', '*=', "*", "/=", "/", "=", ",", ";", "(", ")", "{", "}", "|", ">>", "<<", "&", "^"]

        # INVALID rule indicator
        self.INVALID = -1 

        # Dictionaries used for storing first and follow sets 
        self.FirstDict = dict()
        self.FollowDict = dict()



    def Run(self):
        # Read grammer from input file and intialize grammer related variables 
        self.ReadGrammer()

        # Calculate "First Set" for all nonterminals and print it
        self.FindAllFirsts()
        print("Firsts:")
        self.PrintFirsts()
        print("________________________________________________________________________________")

        # Calculate "Follow Set" for all nonterminals and print it
        self.FindAllFollows()
        print("Follows:")
        self.PrintFollows()
        print("________________________________________________________________________________")
        
        # Calculate "Prdicted Set" for each rule and print it
        self.FindAllPredicts()
        print("Predicts:")
        self.print_predicts()
        print("________________________________________________________________________________")

        # Fills "Parse Table" according to calculted "Predicted Set" and print "Parse Table"
        self.FillParseTable()
        print("Parse Table:")
        self.PrintParseTable()
        print()
        
        # Prints variables that is needed for parser for parsing into the output file 
        self.OutputFile.write("#ifndef PARSE_TABLE_H\n")
        self.OutputFile.write("#define PARSE_TABLE_H\n")
        self.OutputFile.write("#include \"scanner.h\"\n")
        self.OutputFile.write("#define RULES_COUNT " + str(len(self.LhsList)) + "\n")
        self.OutputFile.write("#define TERMINAL_COUNT " + str(len(list(self.TerminalSet))) + "\n")
        self.OutputFile.write("#define NONETERMINAL_COUNT " + str(len(list(self.NonTerminalList))) + "\n")
        self.OutputFile.write("#define START_VARIABLE " + "\"" + self.Start +"\"\n")
        self.OutputFile.write("#define MAX_RHS_LEN "  + str(self.MAXIMUM_RHS_LEN) +"\n")


        # Prints Rules into output file 
        self.WriteLhsList()
        self.WriteRhsList()
        
        # Prints size of each Rhs into output file 
        self.WriteRhsSize()

        # Prints noneterminals and Terminal into output file 
        self.WriteNoneTermianlList()
        self.WriteTerminalList()

        # Prints "Parse Table" into output file
        self.WriteParseTable()


        self.OutputFile.write("#endif\n")

        # Closes Grammer Input File 
        self.GrammerFile.close()

        # Closes Output File 
        self.OutputFile.close()


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

        # Assign top variale an invalid vlaue 
        Top = ""

        # Read input
        Tokens, CurrentIn = Read(Tokens)

        # Temporary vlaues counter initialized with 0 value 
        TempCounter = 0 
        
        # While Stack is not empty repeat 
        while Top != "$":

            # Read top of stack 
            Top = GetTop(Stack)

            
            if self.IsNoneTerminal(Top):
                Id = self.ParseTable[self.GetNoneTerminalId(Top)][self.GetTerminalId(CurrentIn)]

                # Error Handling 
                if Id == -1: 
                    print("1)Error in input!")
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
                else:
                    Op0 = MatchedStack.pop()
                    if Top == "@PRINT":
                        print(Top,"\t", Op0 )
                    elif Top == "@MOV":
                        Op1 = MatchedStack.pop()
                        print(Top,"\t", Op1, ", ", Op0 )
                    else:
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

    
                 

            
    def ReadGrammer(self):
        Flag = 1
        Counter = 0
        for Line in self.GrammerFile:
            Line = Line.strip()
            if Line == "" or Line[0] == "#":
                continue
            L = Line.split("->")
            Lhs = L[0]
            self.NonTerminalSet.add(Lhs)
            self.LhsList.append(Lhs)


            Rhs = L[1].split(" ")
            self.RhsList.append(Rhs)
            for X in Rhs:
                if not self.IsNoneTerminal(X) and not self.IsSemanticRule(X) and not X=="eps":
                    self.TerminalSet.add(X)
            
        


            if Flag:
                Flag = 0
                self.Start = Lhs
            self.MAXIMUM_RHS_LEN = max(self.MAXIMUM_RHS_LEN, len(Rhs))

            Counter += 1

        self.TerminalSet.add("$")
        
        self.NonTerminalList = list(self.NonTerminalSet)
        self.TerminalList = list(self.TerminalSet)






    def WriteLhsList(self):

        self.OutputFile.write("const struct _TOKEN Lhs[RULES_COUNT]= \n{\n")
        Counter = 0
        for Lhs in self.LhsList:
            if Counter == len(self.LhsList)-1:
                self.OutputFile.write("\t{NON_TERMINAL, " + "\"" + Lhs + "\"}" + "\n")
            else:
                self.OutputFile.write("\t{NON_TERMINAL, " + "\"" + Lhs + "\"}" + ",\n")
            Counter +=1
        self.OutputFile.write("};\n")

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
        self.OutputFile.write("const struct _TOKEN Rhs[RULES_COUNT][MAX_RHS_LEN]= \n{\n")
        Counter =0
        for Rhs in self.RhsList:
            self.OutputFile.write("\t{")

            C = 0
            for Var in Rhs:
                if C == len(Rhs) -1:
                    self.OutputFile.write("{"+self.GetType(Var) +", "+"\"" + Var + "\"}" )
                else:
                    self.OutputFile.write("{"+self.GetType(Var) +", "+"\"" + Var + "\"}," )
                C += 1

            if Counter == len(self.RhsList)-1:
                self.OutputFile.write("}\n")
            else:
                self.OutputFile.write("},\n")
            Counter+= 1

        self.OutputFile.write("};\n")

    def WriteRhsSize(self):
        self.OutputFile.write("const unsigned int RhsSize[RULES_COUNT]= \n{\n")
        Counter =0
        for Rhs in self.RhsList:            
            if Counter == len(self.RhsList)-1:
               self.OutputFile.write( str(len(Rhs)) + "\n" )
            else:
                self.OutputFile.write( str(len(Rhs)) + ",\n" )
            Counter+= 1

        self.OutputFile.write("};\n")

    def WriteTerminalList(self):
        self.OutputFile.write("const char* TerminalMap[TERMINAL_COUNT]= \n{\n")
        Counter = 0
        for X in self.TerminalList:
            if Counter == len(self.TerminalList)-1:
                self.OutputFile.write("\"" + X + "\"" + "\n")
            else:
                self.OutputFile.write("\"" + X + "\"" + ",\n")
            Counter +=1
        self.OutputFile.write("};\n")

    def WriteNoneTermianlList(self):
        self.OutputFile.write("const char* NoneTerminalMap[NONETERMINAL_COUNT]= \n{\n")
        Counter = 0
        for X in self.NonTerminalList:
            if Counter == len(self.NonTerminalList)-1:
                self.OutputFile.write("\"" + X + "\"" + "\n")
            else:
                self.OutputFile.write("\"" + X + "\"" + ",\n")
            Counter +=1
        self.OutputFile.write("};\n")

    def WriteParseTable(self):
        self.OutputFile.write("const int ParseTable[NONETERMINAL_COUNT][TERMINAL_COUNT]= \n{\n")
        i = 0
        for X in self.NonTerminalList:
            j = 0
            self.OutputFile.write("\t{")
            for y in self.TerminalList:
                self.OutputFile.write(str(self.ParseTable[i][j]))
                if j != len(self.TerminalList)-1:
                    self.OutputFile.write("\t\t,")
                j += 1

            if i == len(self.NonTerminalList)-1:
                self.OutputFile.write("\t}\n")

            else:
                self.OutputFile.write("\t},\n")
            i +=1
        self.OutputFile.write("};\n")
        


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
                    if self.ParseTable[i][j] == self.INVALID:
                        self.ParseTable[i][j] = RuleId

                    else:
                        print("Error! Input grammer is not LL1.")
                        exit()

                j += 1

            RuleId += 1

    def PrintParseTable(self):
        print("\t", end = "")
        for j in range(len(self.TerminalList)):
            print(self.TerminalList[j], end= "\t ")
        print()
        for i in range(len(self.NonTerminalList)):
            print(self.NonTerminalList[i], end= "\t")
            for j in range(len(self.TerminalList)):
                if self.ParseTable[i][j] == self.INVALID:
                    print(".", end= "\t ")
                else:
                    print(self.ParseTable[i][j], end= "\t ")
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
        while self.IsSemanticRule(Rhs[X]):
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

parser = Parser()
parser.Run()
Tokens = ['print', '(', '_hex',  ')', ';', '$']
Stack = parser.Parse(Tokens)
print(Stack)
