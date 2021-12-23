from os import write
import random

depth = 0
MAX_DEPTH = 20

def S():
    r = random.randrange(2)
    
    
    global depth
    depth += 1
    if depth>=MAX_DEPTH:
        r = 1

    if r==0:
        res = STATEMENT() + ' ' + S()
        depth -= 1
        return res
    elif r==1:
        res = ''
        depth -= 1
        return res

def STATEMENT():
    r = random.randrange(8)
    
    global depth
    depth += 1

    if r==0:
        res = IF_STATEMENT()
        depth -= 1
        return res
    elif r==1:
        res = WHILE_STATEMENT()
        depth -= 1
        return res
    elif r==2:
        res = DO_WHILE_STATEMENT()
        depth-=1
        return res
    elif r==3:
        res = FOR_STATEMENT()
        depth-=1
        return res
    elif r== 4:
        res = ASSIGN_STATEMENT() + ";"
        depth-=1
        return res
    elif r== 5:
        res = CALL_FUNC_STATEMENT() + ";"
        depth-=1
        return res
    elif r== 6:
        res = "break ;"
        depth-=1
        return res
    elif r== 7:
        res = "continue ;"
        depth-=1
        return res

def CALL_FUNC_STATEMENT():
    res = ''
    return res

def ASSIGN_STATEMENT():
    global depth
    depth+=1
    res = L_VALUE() + ' = ' + EXPRESSION() + ' ' + NULL()
    depth-=1
    return res


def IF_STATEMENT():
    global depth
    depth+=1
    res = 'if (' + BOOLEAN_EXPRESSION() +')  {' + S() + '}' + ELSIF_STATEMENT() + ELSE_STATEMENT() + END_OF_IF()
    depth-=1
    return res
    
def ELSIF_STATEMENT():
    r=random.randrange(2)
    
    global depth
    depth += 1
    if depth>=MAX_DEPTH:
        r = 1
    
    if r==0:
        res = 'elsif (' + BOOLEAN_EXPRESSION() + ') {' + S() + '}' + ELSIF_STATEMENT()
        depth-=1
        return res
    elif r==1:
        res = ELSIF_STATEMENTP()
        depth-=1
        return res

def ELSIF_STATEMENTP():
    res = ''
    return res

def ELSE_STATEMENT():
    r=random.randrange(2)
    
    global depth
    depth += 1
    if depth>=MAX_DEPTH:
        r = 1

    if r==0:
        res = 'else {' + S() + '}'
        depth-=1
        return res
    elif r==1: 
        res = ''
        depth-=1
        return res

def END_OF_IF():
    res = ''
    return res

def WHILE_STATEMENT():
    global depth
    depth+=1
    res = 'while  (' + BOOLEAN_EXPRESSION() + ')  { ' + S() + ' }' 
    depth-=1
    return res

def DO_WHILE_STATEMENT():
    global depth
    depth+=1
    res = 'do  {' + S() + '} while ( ' + BOOLEAN_EXPRESSION() + ') ;' 
    depth-=1
    return res

def FOR_STATEMENT():
    global depth
    depth+=1
    res = 'for (' + SIMPLE_ASSIGNMENT() + '; ' + BOOLEAN_EXPRESSION() + ';' +  INC_DEC() + ') { ' + S() + '}'
    depth-=1
    return res

def SIMPLE_ASSIGNMENT():
    r=random.randrange(2)
    
    global depth
    depth += 1
    if depth>=MAX_DEPTH:
        r = 1

    if r==0:
        res = L_VALUE() + '= ' + EXPRESSION()  + SIMPLE_ASSIGNMENTP()
        depth-=1
        return res
    elif r==1:
        res = '' 
        depth-=1
        return res

def SIMPLE_ASSIGNMENTP():
    res = ''
    return res

def INC_DEC():
    global depth
    depth+=1
    res = L_VALUE() + INC_DECP()
    depth-=1
    return res

def INC_DECP():
    global depth
    depth+=1
    res = '++' + INCP()
    depth-=1
    return res

def INC_DECP():
    global depth
    depth+=1
    res = '--' + DECP()
    depth-=1
    return res

def INCP():
    res = ''
    return res

def DECP():
    res = ''
    return res

def INC_DECP():
    res = ''
    return res

def BOOLEAN_EXPRESSION():
    res = ''
    return res


def EXPRESSION():
    global depth
    depth+=1
    res = E1() + E0P()
    depth-=1
    return res

def E0P():
    r = random.randrange(2)
    
    global depth
    depth += 1
    if depth>=MAX_DEPTH:
        r = 1

    if r==0:
        res = ' | ' + E1() +  E0P() 
        depth-=1
        return res
    elif r==1:
        res = ''  
        depth-=1
        return res


def E1():
    global depth
    depth+=1
    res = E2() + E1P()
    depth-=1
    return res

def E1P():
    r=random.randrange(2)
    
    global depth
    depth += 1
    if depth>=MAX_DEPTH:
        r = 1

    if r==0:
        res = ' ^ ' + E2() +  E1P() 
        depth-=1
        return res
    elif r==1:
        res = ''
        depth-=1
        return res

def E2():
    global depth
    depth+=1
    res = E3() + E2P()
    depth-=1
    return res

def E2P():
    r=random.randrange(2)

    global depth
    depth += 1
    if depth>=MAX_DEPTH:
        r = 1

    if r==0:
        res = ' & ' +  E3()  + E2P() 
        depth-=1
        return res
    elif r==1:
        res = ''  
        depth-=1
        return res

def E3():
    global depth
    depth+=1
    res = E4() + E3P()
    depth-=1
    return res

def E3P():
    r=random.randrange(2)
    
    global depth
    depth += 1
    if depth>=MAX_DEPTH:
        r = 1

    if r==0:
        res = ' >> ' + E4() + E3P()
        depth-=1
        return res
    elif r==1:
        res = ''  
        depth-=1
        return res

def E4():
    global depth
    depth+=1
    res = E5()  + E4P()
    depth-=1
    return res

def E4P():
    r=random.randrange(2)
    
    global depth
    depth += 1
    if depth>=MAX_DEPTH:
        r = 1

    if r==0:
        res = ' << ' + E5() + E4P() 
        depth-=1
        return res
    elif r==1:
        res = ''  
        depth-=1
        return res

def E5():
    global depth
    depth+=1
    res = E6() + E5P()
    depth-=1
    return res

def E5P():
    r=random.randrange(2)
    
    global depth
    depth += 1
    if depth>=MAX_DEPTH:
        r = 1

    if r==0:
        res = ' + ' + E6() + E5P()
        depth-=1
        return res
    elif r==1:
        res = ''  
        depth-=1
        return res


def E6():
    global depth
    depth+=1
    res = E7() + E6P()
    depth-=1
    return res

def E6P():
    r=random.randrange(2)
    
    global depth
    depth += 1
    if depth>=MAX_DEPTH:
        r = 1

    if r==0:
        res = ' - ' + E7()  + E6P()
        depth-=1
        return res
    elif r==1:
        res = ''  
        depth-=1
        return res

def E7():
    global depth
    depth+=1
    res = E8() + E7P()
    depth-=1
    return res

def E7P():
    r=random.randrange(2)
    
    global depth
    depth += 1
    if depth>=MAX_DEPTH:
        r = 1

    if r==0:
        res = ' * ' + E8() + E7P() 
        depth-=1
        return res
    elif r==1:
        res = ''  
        depth-=1
        return res


def E8():
    global depth
    depth+=1
    res = E9() + E8P()
    depth-=1
    return res

def E8P():
    r=random.randrange(2)

    global depth
    depth += 1
    if depth>=MAX_DEPTH:
        r = 1

    if r==0:
        res = ' / ' + E9() + E8P()
        depth-=1
        return res
    elif r==1:
        res = ''  
        depth-=1
        return res


def E9():
    global depth
    depth+=1
    res = E10() + E9P()
    depth-=1
    return res

def E9P():
    r=random.randrange(2)
    
    global depth
    depth += 1
    if depth>=MAX_DEPTH:
        r = 1

    if r==0:
        res = ' % ' + E10()  + E9P()
        depth-=1
        return res
    elif r==1:
        res = ''  
        depth-=1
        return res

def E10():
    global depth
    depth+=1
    res = E12()
    depth-=1
    return res



def E12():
    r=random.randrange(0,8)

    global depth
    depth += 1
    if depth>=MAX_DEPTH:
        r = 1

    
    if r==0:
        res = '(' + EXPRESSION() + ')'
        depth-=1
        return res
    elif r==1:
        ri = random.randrange(0,20)     
        res = str(hex(ri)) # hex
        depth-=1
        return res
    elif r==2:
        ri = random.randrange(0,20)  
        res = '0n' + str(ri) # decimal
        depth-=1
        return res
    elif r==3:
        ri = random.randrange(0,20)      
        res = str(oct(ri))  # octal
        depth-=1
        return res
    elif r==4:
        ri = random.randrange(0,20)     
        res = '0y' + str(bin(ri))[2:]  # binary
        depth-=1
        return res
    elif r==5:
        res = '-' + E12() + E13()
        depth-=1
        return res
    elif r==6:      
        res = '+' + E12() + E13()
        depth-=1
        return res
    elif r==7:
        res = '~' + E12() + E13()
        depth-=1
        return res
    elif r==8:
        res = '@rax' # register
        depth-=1
        return res
    elif r==9:
        res = 'x' # id
        depth-=1
        return res
    elif r==10:
        res = 'pid' # sample pseudo register, can be added more options later
        depth-=1
        return res

def E13():
    res = ''
    return res

def L_VALUE():
    r = random.randrange(2)

    if r==0:
        res = 'x'   # id
        return res
    elif r==1:
        res = '@rax' # register
        return res

def NULL():
    res = ''
    return res

def tohex(val, nbits):
  return hex((val + (1 << nbits)) % (1 << nbits))

def evaluate(s):
    s2 = s.replace('0n','').replace('0y','0b').replace('/','//')
    v = '$error$'
    
    try:
        v = eval(s2)
        v = tohex(v, 64)
        v = v.replace('0x', '')
    except:
        pass
    return v


'''
if __name__ == '__main__':
#    global depth
    
    f = open('script-test-cases.txt', 'w')
    counter = 1

    initCounter = counter
    while counter-initCounter<1000:
        depth = 0
        sentence = EXPRESSION()
        if len(sentence)<=150:
            res = 'x = ' + sentence + '; test_statement(x);'
            val = evaluate(sentence)
            if type(val)=='int' and not(abs(val)<=65536):
                continue
            f.write(str(counter)+'\n')
            f.write(res+'\n')
            f.write(str(val)+'\n')
            f.write('$end$'+'\n')
            print(counter)
            print(res)
            print(val)
            print()
            counter+=1
    
    f.close()
'''
if __name__ == '__main__':
#    global depth
    
    f = open('script-test-cases.txt', 'w')
    counter = 1

    initCounter = counter
    while counter-initCounter<1000:
        depth = 0
        sentence = FOR_STATEMENT()
        if len(sentence)<=150:
            res = sentence 
            val = evaluate(sentence)
            if type(val)=='int' and not(abs(val)<=65536):
                continue
            f.write(str(counter)+'\n')
            f.write(res+'\n')
            f.write(str(val)+'\n')
            f.write('$end$'+'\n')
            print(counter)
            print(res)
            print(val)
            print()
            counter+=1
    
    f.close()