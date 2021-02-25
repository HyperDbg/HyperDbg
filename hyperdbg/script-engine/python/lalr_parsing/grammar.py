class NonTerminal:
    def __init__(self, name, productions):
        # Creates an instance of a NonTerminal that represents a set of
        # grammar productions for a non-terminal.
        #
        # Keyword arguments:
        # name -- the name of the non-terminal
        # productions -- a list whose elements can be:
        #   1. Lists of objects of type NTerm or str.
        #      The str elements represent terminal grammar symbols.
        #   2. a str with space-separated words. These words represent grammar symbols (T and NT)
        #
        self.name = name
        self.productions = [(x.split() if isinstance(x, str) else x) for x in productions]

    def __repr__(self):
        return 'NonTerminal(' + repr(self.name) + ')'

    def __str__(self):
        return self.name

    def stringify(self, pretty=True):
        title = '%s: ' % self.name

        if pretty:
            separator = '\n%s| ' % (' ' * len(self.name))
        else:
            separator = ' | '

        def strprod(prod):
            return ' '.join(str(sym) for sym in prod)

        rules = separator.join(strprod(prod) for prod in self.productions)

        return title + rules


class Grammar:
    def __init__(self, nonterms, start_nonterminal=None):
        # Grammar start symbol
        if start_nonterminal is None or start_nonterminal not in nonterms:
            start_nonterminal = nonterms[0]

        # Tuple of non-terminals
        self.nonterms = tuple([NonTerminal(START_SYMBOL, [[start_nonterminal.name]])] +
                              nonterms)
        # Tuple of terminals
        self.terminals = ()
        # Tuple of symbols (non-terminals + terminals)
        self.symbols = ()
        # Tuple of enumerated NT's productions
        self.productions = ()
        # Enumeration offset for a given NT
        self.nonterm_offset = {}
        # First sets for every grammar symbol
        self.__first_sets = {}

        # Update the references of each production and, while at it, recognize terminal symbols
        nonterminal_by_name = {nt.name: nt for nt in self.nonterms}
        for nt in self.nonterms:
            for prod in nt.productions:
                for idx in range(len(prod)):
                    symbol = prod[idx]

                    if isinstance(symbol, str):
                        if symbol in nonterminal_by_name:
                            prod[idx] = nonterminal_by_name[symbol]
                        else:
                            self.terminals += tuple([symbol])
                    elif isinstance(symbol, NonTerminal):
                        if symbol not in self.nonterms:
                            msg = 'Non-terminal %s is not in the grammar' % repr(symbol)
                            raise KeyError(msg)
                    else:
                        msg = "Unsupported type '%s' inside of production" % type(symbol).__name__
                        raise TypeError(msg)

        self.terminals = tuple(set(self.terminals))
        self.symbols = self.nonterms + self.terminals

        # Enumerate grammar productions
        for nt in self.nonterms:
            self.nonterm_offset[nt] = len(self.productions)
            self.productions += tuple((nt.name, prod) for prod in nt.productions)

        self.__build_first_sets()

    def first_set(self, x):
        result = set()

        if isinstance(x, str):
            result.add(x)
        elif isinstance(x, NonTerminal):
            result = self.__first_sets[x]
        else:
            skippable_symbols = 0
            for sym in x:
                fs = self.first_set(sym)
                result.update(fs - {None})
                if None in fs:
                    skippable_symbols += 1
                else:
                    break

            if skippable_symbols == len(x):
                result.add(None)

        return frozenset(result)

    def __build_first_sets(self):
        #  Starting First sets values
        for s in self.symbols:
            if isinstance(s, str):
                self.__first_sets[s] = {s}
            else:
                self.__first_sets[s] = set()
                if [] in s.productions:
                    self.__first_sets[s].add(None)

        # Update the sets iteratively (see Dragon Book, page 221)
        repeat = True
        while repeat:
            repeat = False
            for nt in self.nonterms:
                curfs = self.__first_sets[nt]
                curfs_len = len(curfs)

                for prod in nt.productions:
                    skippable_symbols = 0
                    for sym in prod:
                        fs = self.__first_sets[sym]
                        curfs.update(fs - {None})
                        if None in fs:
                            skippable_symbols += 1
                        else:
                            break
                    if skippable_symbols == len(prod):
                        curfs.add(None)

                if len(curfs) > curfs_len:
                    repeat = True

        # Freeze the sets
        self.__first_sets = {x: frozenset(y) for x, y in self.__first_sets.items()}

    def stringify(self, indexes=True):
        lines = '\n'.join(nt.stringify() for nt in self.nonterms)
        if indexes:
            lines = '\n'.join(RULE_INDEXING_PATTERN % (x, y)
                              for x, y in enumerate(lines.split('\n')))
        return lines

    def __str__(self):
        return self.stringify()


RULE_INDEXING_PATTERN = '%-5d%s'
START_SYMBOL = '$accept'
EOF_SYMBOL = "'$end'"
FREE_SYMBOL = '$#'
