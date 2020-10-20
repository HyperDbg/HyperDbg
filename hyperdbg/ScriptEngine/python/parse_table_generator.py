import re

# Util functions 
def get_top(l):
    return l[len(l)-1]

def read(tokens):
    x = tokens[0]
    tokens = tokens[1:]
    return tokens, x 






class Parser:
    def __init__(self):
        self.grammer_file = open("Grammer.txt", "r")
        self.output_file = open("..\src\parse_table.h", "w")
        self.rhs_list = []
        self.lhs_list = []
        self.terminal_set = set()
        self.nonterminal_set = set()
        self.start = ""
        self.maximum_rhs_len = 0


        self.SPECIAL_TOKENS = ['%', '++', '+=', '+', '--', '-=', '-', '*=', "*", "/=", "/", "=", ",", ";", "(", ")", "{", "}", "|", ">>", "<<", "&", "^"]

        self.first_dict = dict()
        self.follow_dict = dict()

        self.INVALID = -1 

    def run(self):
        self.read_grammer()

        self.find_all_firsts()
        print("Firsts:")
        self.print_firsts()
        print("________________________________________________________________________________")

        self.find_all_follows()
        print("Follows:")
        self.print_follows()
        print("________________________________________________________________________________")
        

        self.find_all_perdicts()
        print("Predicts:")
        self.print_predicts()
        print("________________________________________________________________________________")


        self.fill_parse_table()
        print("Parse Table:")
        self.print_parse_table()
        print()
        

        self.output_file.write("#ifndef PARSE_TABLE_H\n")
        self.output_file.write("#define PARSE_TABLE_H\n")
        self.output_file.write("#include \"scanner.h\"\n")
        self.output_file.write("#define RULES_COUNT " + str(len(self.lhs_list)) + "\n")
        self.output_file.write("#define TERMINAL_COUNT " + str(len(list(self.terminal_set))) + "\n")
        self.output_file.write("#define NONETERMINAL_COUNT " + str(len(list(self.nonterminal_list))) + "\n")
        self.output_file.write("#define START_VARIABLE " + "\"" + self.start +"\"\n")
        self.output_file.write("#define MAX_RHS_LEN "  + str(self.maximum_rhs_len) +"\n")


        self.write_lhs_list()
        self.write_rhs_list()
        self.write_rhs_size()
        self.write_noneterminal_list()
        self.write_terminal_list()
        self.write_parse_table()


        self.output_file.write("#endif\n")

        self.grammer_file.close()
        self.output_file.close()


    def parse(self, tokens):
        stack = []
        matched_stack = []
        stack.append("$")
        stack.append(self.start)
        top = ""
        tokens, current_in = read(tokens)
        t_counter = 0 
        
        while top != "$":
            top = get_top(stack)

            # print(stack)
            
            if self.isNoneTerminal(top):
                id = self.parse_table[self.get_noneterminal_id(top)][self.get_terminal_id(current_in)]

                # Error Handling 
                if id == -1: 
                    print("1)Error in input!")
                    exit()

                stack.pop()
                

                rhs = self.rhs_list[id]
                if rhs != ["eps"]:
                    for symbol in reversed(rhs):
                        stack.append(symbol)

                
            elif self.isSemanticRule(top):
                # TODO: Handle Semantic rules
                if top == "@PUSH":
                    stack.pop()
                    top = get_top(stack)
                    if top == current_in:
                        matched_stack.append(top)
                        current_in = tokens[0]
                        tokens = tokens[1:]
                    else: 
                        print("2)Error in input!")
                        exit()
                        
                else:
                    
                    
                    
                    op0 = matched_stack.pop()
  
                    op1 = matched_stack.pop()

                    if top == "@MOV":
                        print(top,"\t", op1, ", ", op0 )
                    else:
                        matched_stack.append("t" + str(t_counter))
                        print(top, "\t", "t"+ str(t_counter), ", ", op1, ",", op0 )
                        t_counter += 1
                    
                # print("matched stack :", end ="")
                # print(matched_stack)

                stack.pop()

            else: # Terminal 
                # TODO: Handle Error 
                current_in = tokens[0]
                if (len(tokens) > 1):
                    tokens = tokens[1:]
                stack.pop()
            # print(top)

        return matched_stack

    # def code_gen(self, input_stack):
    #     print("input stack:", input_stack)
    #     if len(input_stack) < 3:
    #         return input_stack
    #     top_idx = len(input_stack) - 1
    #     first_op_idx = len(input_stack) - 2
    #     second_op_idx = len(input_stack) - 3

    #     output_stack = []

    #     if not self.isSemanticRule(input_stack[top_idx]):
    #         print("Error!")
    #         exit()
    #     else:
    #         output_stack.append(input_stack.pop())
    #         if not self.isSemanticRule(input_stack[first_op_idx]): # Operand
    #             output_stack.append(input_stack.pop())
    #         else: # Operator
    #             output_stack.extend(self.code_gen(input_stack))

    #     if not self.isSemanticRule(input_stack[second_op_idx]): # Operand 
    #         output_stack.append(input_stack.pop())
    #     else: #Operator  
    #         output_stack.extend(self.code_gen(input_stack))
    #     print("output stack:", output_stack)
    #     return output_stack
                 

            
    def read_grammer(self):
        flag = 1
        counter = 0
        for line in self.grammer_file:
            line = line.strip()
            if line == "" or line[0] == "#":
                continue
            l = line.split("->")
            lhs = l[0]
            self.nonterminal_set.add(lhs)
            self.lhs_list.append(lhs)


            rhs = l[1].split(" ")
            self.rhs_list.append(rhs)
            for x in rhs:
                if not self.isNoneTerminal(x) and not self.isSemanticRule(x) and not x=="eps":
                    self.terminal_set.add(x)
            
        


            if flag:
                flag = 0
                self.start = lhs
            self.maximum_rhs_len = max(self.maximum_rhs_len, len(rhs))

            counter += 1

        self.terminal_set.add("$")
        
        self.nonterminal_list = list(self.nonterminal_set)
        self.terminal_list = list(self.terminal_set)






    def write_lhs_list(self):

        self.output_file.write("const struct _TOKEN Lhs[RULES_COUNT]= \n{\n")
        counter = 0
        for lhs in self.lhs_list:
            if counter == len(self.lhs_list)-1:
                self.output_file.write("\t{NON_TERMINAL, " + "\"" + lhs + "\"}" + "\n")
            else:
                self.output_file.write("\t{NON_TERMINAL, " + "\"" + lhs + "\"}" + ",\n")
            counter +=1
        self.output_file.write("};\n")

    def get_type(self,var):
        if self.isNoneTerminal(var):
            return "NON_TERMINAL"
        elif self.isSemanticRule(var):
            return "SEMANTIC_RULE"

        elif var == "eps":
            return "EPSILON"

        elif var in self.SPECIAL_TOKENS:
            return "SPECIAL_TOKEN"
        elif var[0] == "_":
            return var[1:].upper()
        else:
            return "KEYWORD"


    def write_rhs_list(self):
        self.output_file.write("const struct _TOKEN Rhs[RULES_COUNT][MAX_RHS_LEN]= \n{\n")
        counter =0
        for rhs in self.rhs_list:
            self.output_file.write("\t{")

            c = 0
            for var in rhs:
                if c == len(rhs) -1:
                    self.output_file.write("{"+self.get_type(var) +", "+"\"" + var + "\"}" )
                else:
                    self.output_file.write("{"+self.get_type(var) +", "+"\"" + var + "\"}," )
                c += 1

            if counter == len(self.rhs_list)-1:
                self.output_file.write("}\n")
            else:
                self.output_file.write("},\n")
            counter+= 1

        self.output_file.write("};\n")

    def write_rhs_size(self):
        self.output_file.write("const unsigned int RhsSize[RULES_COUNT]= \n{\n")
        counter =0
        for rhs in self.rhs_list:            
            if counter == len(self.rhs_list)-1:
               self.output_file.write( str(len(rhs)) + "\n" )
            else:
                self.output_file.write( str(len(rhs)) + ",\n" )
            counter+= 1

        self.output_file.write("};\n")

    def write_terminal_list(self):
        self.output_file.write("const char* TerminalMap[TERMINAL_COUNT]= \n{\n")
        counter = 0
        for x in self.terminal_list:
            if counter == len(self.terminal_list)-1:
                self.output_file.write("\"" + x + "\"" + "\n")
            else:
                self.output_file.write("\"" + x + "\"" + ",\n")
            counter +=1
        self.output_file.write("};\n")

    def write_noneterminal_list(self):
        self.output_file.write("const char* NoneTerminalMap[NONETERMINAL_COUNT]= \n{\n")
        counter = 0
        for x in self.nonterminal_list:
            if counter == len(self.nonterminal_list)-1:
                self.output_file.write("\"" + x + "\"" + "\n")
            else:
                self.output_file.write("\"" + x + "\"" + ",\n")
            counter +=1
        self.output_file.write("};\n")

    def write_parse_table(self):
        self.output_file.write("const int ParseTable[NONETERMINAL_COUNT][TERMINAL_COUNT]= \n{\n")
        i = 0
        for x in self.nonterminal_list:
            j = 0
            self.output_file.write("\t{")
            for y in self.terminal_list:
                self.output_file.write(str(self.parse_table[i][j]))
                if j != len(self.terminal_list)-1:
                    self.output_file.write("\t\t,")
                j += 1

            if i == len(self.nonterminal_list)-1:
                self.output_file.write("\t}\n")

            else:
                self.output_file.write("\t},\n")
            i +=1
        self.output_file.write("};\n")
        


    def find_all_firsts(self):
        
        self.first_dict = {}
        for symbol in self.nonterminal_list:
            self.first_dict[symbol] = set()

        t = 0 
        while True:
            updated = False 
            i = 0 
            for lhs in self.lhs_list:
                rhs = self.rhs_list[i]
                temp = set(self.first_dict[lhs])

                
                if rhs[0] == "eps":
                    pass
                elif self.isNoneTerminal(rhs[0]):
                    self.first_dict[lhs] = self.first_dict[lhs].union(self.first_dict[rhs[0]])
                    p = 0
                    while self.isnullable(rhs[p]):
                        self.first_dict[lhs] = self.first_dict[lhs].union(self.first_dict[rhs[p+1]])
                        p += 1
                elif self.isSemanticRule(rhs[0]):
                    if self.isNoneTerminal(rhs[0]):
                        self.first_dict[lhs] = self.first_dict[lhs].union(self.first_dict[rhs[1]])
                    else: 
                        self.first_dict[lhs].add(rhs[1])
                else:
                    self.first_dict[lhs].add(rhs[0])
                i += 1
               
                if temp != self.first_dict[lhs]:
                    updated = True
            p += 1  
            if not updated:
                break;    
            


    def print_firsts(self):
        for id in self.first_dict:
            print(id, end=": ")
            for x in self.first_dict[id]:
                print(x,end = " ")
            print()

    def get_noneterminal_id(self, nonterminal):
        for i in range(len(self.nonterminal_list)):
            if nonterminal == self.nonterminal_list[i]:
                return i

        return -1
    def get_terminal_id(self, terminal):
        for i in range(len(self.terminal_list)):
            if terminal == self.terminal_list[i]:
                return i

        return -1


    def fill_parse_table(self):
        self.parse_table = [[self.INVALID for y in range(len(self.terminal_list))] for x in range(len(self.nonterminal_list))]

        rule_id = 0 
        for lhs in self.lhs_list:
            i = self.get_noneterminal_id(lhs)

            j = 0
            for terminal in self.terminal_list:
                if terminal in self.predict_dict[rule_id]:
                    if self.parse_table[i][j] == self.INVALID:
                        self.parse_table[i][j] = rule_id

                    else:
                        print("Error! Input grammer is not LL1.")
                        exit()

                j += 1

            rule_id += 1

    def print_parse_table(self):
        print("\t", end = "")
        for j in range(len(self.terminal_list)):
            print(self.terminal_list[j], end= "\t ")
        print()
        for i in range(len(self.nonterminal_list)):
            print(self.nonterminal_list[i], end= "\t")
            for j in range(len(self.terminal_list)):
                if self.parse_table[i][j] == self.INVALID:
                    print(".", end= "\t ")
                else:
                    print(self.parse_table[i][j], end= "\t ")
            print()
                

    def find_all_perdicts(self):
        self.predict_dict = {}
        for i in range(len(self.lhs_list)):
            self.predict_dict[i] = set()

        i = 0
        for lhs in self.lhs_list:
            rhs = self.rhs_list[i]
            is_right_nullable = True
            # print(lhs, end= ": ")
            # print(rhs)

            # if rhs[0] == "eps":
            #     pass
            for symbol in rhs:
                if self.isSemanticRule(symbol):
                    pass
                
                elif symbol == "eps":
                    is_right_nullable = True
                    break
                elif self.isNoneTerminal(symbol):
                    self.predict_dict[i] |= self.first_dict[symbol]
                    if not self.isnullable(symbol):
                        is_right_nullable = False
                        break
                else:
                    self.predict_dict[i].add(symbol)
                    is_right_nullable = False
                    break
            
            if is_right_nullable:
                self.predict_dict[i] |= self.follow_dict[lhs]
            i += 1
               

    def print_predicts(self):
        for key in self.predict_dict:
            print(key, end=": ")
            for x in self.predict_dict[key]:
                print(x,end = " ")
            print()


    
    def find_all_follows(self):
        self.follow_dict = {}
        for symbol in self.nonterminal_list:
            self.follow_dict[symbol] = set()
            if symbol == self.start:
                self.follow_dict[symbol].add('$')

        t = 0
        while True: 
            updated = False 

            i = 0
            for lhs in self.lhs_list:
                rhs = self.rhs_list[i]

                p = 0
                for symbol in rhs:
                    
                    if self.isNoneTerminal(symbol):
                        
                        temp = set(self.follow_dict[symbol])
                        if p == len(rhs)-1:
                            self.follow_dict[symbol] = self.follow_dict[symbol].union(self.follow_dict[lhs])
                        
                        
                        
                        else:

                            next_var = self.get_next_var(rhs, p)
                            if self.isNoneTerminal(next_var):
                                self.follow_dict[symbol] = self.follow_dict[symbol].union(self.first_dict[next_var])
                                if self.isnullable(next_var):    
                                    self.follow_dict[symbol] = self.follow_dict[symbol].union(self.follow_dict[next_var])                                    
                            elif self.isSemanticRule(next_var):
                                pass      
                            else:
                                self.follow_dict[symbol].add(next_var)
                        
                        if p == len(rhs)-2:
                            next_var = self.get_next_var(rhs, p)
                            
                            if self.isnullable(next_var):
                                self.follow_dict[symbol] = self.follow_dict[symbol].union(self.follow_dict[lhs])
                                self.follow_dict[lhs] = self.follow_dict[lhs].union(self.follow_dict[next_var])
                        
                   

                        if temp != self.follow_dict[symbol]:
                            updated = True
                    p += 1  
                i += 1
            t += 1

            if not updated: 
                break
    def get_next_var(self, rhs, p):
        if p == len(rhs)-1:
            return None
        
        x = p +1
        while self.isSemanticRule(rhs[x]):
            x+=1
        return rhs[x]
        
    def print_follows(self):
        for key in self.follow_dict:
            print(key, end=": ")
            for x in self.follow_dict[key]:
                print(x,end = " ")
            print()

    def isNoneTerminal(self,x):     
        if x[0].isupper():
            return True
        else:
            return False

    def isSemanticRule(self, x):
        if x[0] == '@':
            return True
        else: 
            return False

    def isnullable(self, s):

        if not s[0].isupper():
            return False
        i = 0 
        for lhs in self.lhs_list:
            rhs = self.rhs_list[i]
            
            if lhs == s:

                if rhs[0] == "eps":
                    return True

                p = 0 
                while True:
                    if self.isnullable(rhs[p]):
                        return True
                    else:
                        break
                    p += 1
            i +=1
                    
        return False

parser = Parser()
parser.run()
tokens = ['_id', '=', '(', '_hex', '*', '_hex', ')', '*', '_hex', '*', '_decimal', '*', '_octal', ';', '$']
stack = parser.parse(tokens)
print(stack)
# parser.code_gen(stack)




