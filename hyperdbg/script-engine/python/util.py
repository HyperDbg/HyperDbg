# Returns Top of the list L 
def GetTop(L):
    return L[len(L)-1]

# Remove first element of list and return it 
def Read(Tokens):
    X = Tokens[0]
    Tokens = Tokens[1:]
    return Tokens, X 
