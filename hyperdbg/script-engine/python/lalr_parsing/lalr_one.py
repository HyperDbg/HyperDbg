import lalr_parsing.lr_zero as lr_zero
import lalr_parsing.grammar as grammar
import csv


class ParsingTable:
    def __init__(self, gr):
        self.grammar = gr

        self.terminals = ()  # terminals of 'gr', union grammar.EOF_SYMBOL
        self.nonterms = ()   # non-terminals of 'gr', except grammar.START_SYMBOL

        self.__ccol = ()
        self.n_states = 0  # Taken from the cardinality of the LALR(1) canonical collection

        # goto will be a list of dictionaries in order to easy its usage:
        #   goto[state_id][nonterminal]
        # Each dictionary will map a non-terminal to an int: the index of the next state.
        # If for a given non-terminal there's no transition, it will map to None instead.
        #
        # action will be a list of dictionaries in order to easy its usage:
        #   action[state_id][terminal]
        # Each dictionary will map a terminal to a set of "table entries".
        # The type of a table entry varies according to the kind of entry:
        #   a "shift #state_id" entry     is a 2-tuple ('shift and go to state', state_id)
        #   a "reduce #prod_index" entry  is a 2-tuple ('reduce using rule', prod_index)
        #   an "accept" entry             is just a 2-tuple ('accept', '')
        # As a special case: an "error" entry is detoned just by the empty set of table
        # entries.
        #
        # See Dragonbook, page 265, "Canonical LR(1) parsing tables" for reference.

        self.goto = ()
        self.action = ()

        self.__setup_from_grammar(self.grammar)

    def __setup_from_grammar(self, gr):
        self.terminals = gr.terminals + tuple([grammar.EOF_SYMBOL])
        self.nonterms = gr.nonterms[1:]

        self.__ccol = tuple(get_canonical_collection(gr))
        self.n_states = len(self.__ccol)

        ccol_core = tuple(drop_itemset_lookaheads(x) for x in self.__ccol)
        id_from_core = {ccol_core[i]: i for i in range(len(self.__ccol))}

        self.goto = tuple({x: None for x in self.nonterms} for i in range(self.n_states))
        self.action = tuple({x: set() for x in self.terminals} for i in range(self.n_states))

        # Precalculation of goto values to improve performance
        goto_precalc = tuple(dict() for i in range(self.n_states))
        for symbol in (self.terminals + self.nonterms):
            for state_id in range(self.n_states):
                next_state = goto(gr, self.__ccol[state_id], symbol)
                if len(next_state) == 0:
                    continue
                next_state_id = id_from_core[drop_itemset_lookaheads(next_state)]
                goto_precalc[state_id][symbol] = next_state_id

        for state_id in range(self.n_states):
            for item, next_symbol in self.__ccol[state_id]:
                prod_index, dot = item
                pname, pbody = gr.productions[prod_index]

                if dot < len(pbody):
                    terminal = pbody[dot]
                    if not isinstance(terminal, str) or terminal not in goto_precalc[state_id]:
                        continue

                    next_state_id = goto_precalc[state_id][terminal]
                    self.action[state_id][terminal].add(('S', next_state_id))
                else:
                    if prod_index == 0:
                        # We are dealing with an item of the artificial starting symbol
                        assert(next_symbol == grammar.EOF_SYMBOL)
                        self.action[state_id][grammar.EOF_SYMBOL].add(('accept', ''))
                    else:
                        # We are dealing with a regular non-terminal
                        self.action[state_id][next_symbol].add(('R', prod_index))

            for nt in self.nonterms:
                if nt not in goto_precalc[state_id]:
                    continue
                next_state_id = goto_precalc[state_id][nt]
                self.goto[state_id][nt] = next_state_id

    @staticmethod
    def __stringify_action_entries(term, ent):
        return '\tfor terminal %s: ' % term + \
               ', '.join('%s %s' % (kind, str(arg)) for kind, arg in ent)

    @staticmethod
    def __stringify_goto_entry(nt, sid):
            return '\tfor non-terminal %s: go to state %d' % (str(nt), sid)

    def __stringify_lr_zero_item(self, item):
        prod_index, dot = item

        pname, pbody = self.grammar.productions[prod_index]

        dotted_pbody = pbody[:dot] + ['.'] + pbody[dot:]

        dotted_pbody_str = ' '.join(str(x) for x in dotted_pbody)

        return grammar.RULE_INDEXING_PATTERN % (prod_index, pname + ': ' + dotted_pbody_str)

    def stringify_state(self, state_id):
        state_title = 'State %d\n' % state_id

        items = drop_itemset_lookaheads(kernels(self.__ccol[state_id]))
        items = sorted(items, key=lambda elem: elem[0])

        items_str = '\n'.join('\t' + self.__stringify_lr_zero_item(item) for item in items) + '\n\n'

        actions = [(t, e) for t, e in self.action[state_id].items() if len(e) > 0]
        actions = sorted(actions, key=lambda elem: elem[0])

        actions_str = '\n'.join(self.__stringify_action_entries(t, e) for t, e in actions)
        actions_str += ('\n' if len(actions_str) > 0 else '')

        gotos = [(nt, sid) for nt, sid in self.goto[state_id].items() if sid is not None]
        gotos = sorted(gotos, key=lambda elem: elem[0].name)

        gotos_str = '\n'.join(self.__stringify_goto_entry(nt, sid) for nt, sid in gotos)
        gotos_str += ('\n' if len(gotos_str) > 0 else '')

        action_goto_separator = ('\n' if len(actions_str) > 0 and len(gotos_str) > 0 else '')

        return state_title + items_str + actions_str + action_goto_separator + gotos_str

    def stringify(self):
        states_str = '\n'.join(self.stringify_state(i) for i in range(self.n_states))
        return states_str

    @staticmethod
    def __get_entry_status(e):
        if len(e) <= 1:
            return STATUS_OK

        n_actions = len(frozenset(x for x, y in e))
        return STATUS_SR_CONFLICT if n_actions == 2 else STATUS_RR_CONFLICT

    def get_single_state_conflict_status(self, state_id):
        seq = [self.__get_entry_status(e) for t, e in self.action[state_id].items()]
        return STATUS_OK if len(seq) == 0 else max(seq)

    def get_conflict_status(self):
        return [self.get_single_state_conflict_status(i) for i in range(self.n_states)]

    def is_lalr_one(self):
        seq = self.get_conflict_status()
        return (STATUS_OK if len(seq) == 0 else max(seq)) == STATUS_OK

    def save_to_csv(self, filepath):
        with open(filepath, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile, dialect='excel')

            headers = tuple(' ') + self.terminals + self.nonterms
            writer.writerow(headers)

            def stringify_action_entries(entries):
                return ', '.join(e[0].split()[0][0] + str(e[1]) for e in entries)

            for state_id in range(self.n_states):
                row = [''] * len(headers)
                row[0] = state_id

                for col in range(1, 1 + len(self.terminals)):
                    if not headers[col] in self.action[state_id]:
                        continue
                    row[col] = stringify_action_entries(self.action[state_id][headers[col]])

                for col in range(1 + len(self.terminals), len(headers)):
                    if not headers[col] in self.goto[state_id]:
                        continue
                    row[col] = self.goto[state_id][headers[col]]
                
                writer.writerow(row)


class LrZeroItemTableEntry:
    def __init__(self):
        self.propagates_to = set()
        self.lookaheads = set()

    def __repr__(self):
        pattern = '{ propagatesTo: %s, lookaheads: %s }'
        return pattern % (repr(self.propagates_to), repr(self.lookaheads))


def get_canonical_collection(gr):
    # See Dragonbook, page 272, "Algorithm 4.63"

    # STEP 1
    # ======
    dfa = lr_zero.get_automaton(gr)
    kstates = [lr_zero.kernels(st) for st in dfa.states]
    n_states = len(kstates)

    # STEPS 2, 3
    # ==========
    table = [{item: LrZeroItemTableEntry() for item in kstates[i]} for i in range(n_states)]
    table[0][(0, 0)].lookaheads.add(grammar.EOF_SYMBOL)

    for i_state_id in range(n_states):
        state_symbols = [x[1] for x, y in dfa.goto.items() if x[0] == i_state_id]

        for i_item in kstates[i_state_id]:
            closure_set = closure(gr, [(i_item, grammar.FREE_SYMBOL)])

            for sym in state_symbols:
                j_state_id = dfa.goto[(i_state_id, sym)]

                # For each item in closure_set whose . (dot) points to a symbol equal to 'sym'
                # i.e. a production expecting to see 'sym' next
                for ((prod_index, dot), next_symbol) in closure_set:
                    pname, pbody = gr.productions[prod_index]
                    if dot == len(pbody) or pbody[dot] != sym:
                        continue

                    j_item = (prod_index, dot + 1)
                    if next_symbol == grammar.FREE_SYMBOL:
                        table[i_state_id][i_item].propagates_to.add((j_state_id, j_item))
                    else:
                        table[j_state_id][j_item].lookaheads.add(next_symbol)

    # STEP 4
    # ======
    repeat = True
    while repeat:
        repeat = False
        # For every item set, kernel item
        for i_state_id in range(len(table)):
            for i_item, i_cell in table[i_state_id].items():
                # For every kernel item i_item's lookaheads propagate to
                for j_state_id, j_item in i_cell.propagates_to:
                    # Do propagate the lookaheads
                    j_cell = table[j_state_id][j_item]
                    j_cell_lookaheads_len = len(j_cell.lookaheads)
                    j_cell.lookaheads.update(i_cell.lookaheads)
                    # Check if they changed, so we can decide whether to iterate again
                    if j_cell_lookaheads_len < len(j_cell.lookaheads):
                        repeat = True

    # Build the collection
    # ====================
    result = [set() for i in range(n_states)]
    for i_state_id in range(n_states):
        # Add kernel items
        for i_item, i_cell in table[i_state_id].items():
            for sym in i_cell.lookaheads:
                item_set = (i_item, sym)
                result[i_state_id].add(item_set)
        # Add non-kernel kernel items
        result[i_state_id] = closure(gr, result[i_state_id])

    return result


def closure(gr, item_set):
    result = set(item_set)
    current = item_set

    while len(current) > 0:
        new_elements = []

        for ((prod_index, dot), lookahead) in current:
            pname, pbody = gr.productions[prod_index]
            if dot == len(pbody) or pbody[dot] not in gr.nonterms:
                continue

            nt = pbody[dot]
            nt_offset = gr.nonterm_offset[nt]
            following_symbols = pbody[dot+1:] + [lookahead]
            following_terminals = gr.first_set(following_symbols) - {None}

            for idx in range(len(nt.productions)):
                for term in following_terminals:
                    new_item_set = ((nt_offset + idx, 0), term)
                    if new_item_set not in result:
                        result.add(new_item_set)
                        new_elements += [new_item_set]

        current = new_elements

    return frozenset(result)


def goto(gr, item_set, inp):
    result_set = set()
    for (item, lookahead) in item_set:
        prod_id, dot = item
        pname, pbody = gr.productions[prod_id]
        if dot == len(pbody) or pbody[dot] != inp:
            continue

        new_item = ((prod_id, dot + 1), lookahead)
        result_set.add(new_item)

    result_set = closure(gr, result_set)
    return result_set


def kernels(item_set):
    return frozenset((item, nextsym) for item, nextsym in item_set if item[1] > 0 or item[0] == 0)


def drop_itemset_lookaheads(itemset):
    return frozenset((x[0], x[1]) for x, y in itemset)


STATUS_OK = 0
STATUS_SR_CONFLICT = 1
STATUS_RR_CONFLICT = 2
