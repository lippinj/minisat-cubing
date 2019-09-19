import gzip
from collections import defaultdict


class Cnf:
    """A formula of propositional logic, in conjunctive normal form.

    The conjunctive normal form is a conjunction (AND) of disjunctions
    (OR) of literals (variables or their negations), e.g.:

        (a OR b OR ~d) AND (~b OR c OR e) AND (~b OR ~e OR f) AND ...

    The disjunctions are called "clauses". This object represents the
    variables as positive integers 1,2,... and their negations as the
    additive inverses -1,-2,... .
    """

    def __init__(self, clauses=[], comments=[]):
        """Constructor."""
        # Sanity checks for the input:
        #  1. clauses should be iterable (implicit)
        #  2. each clause should be iterable (implicit)
        #  3. each literal should be a nonzero integer
        for clause in clauses:
            assert all(isinstance(L, int) for L in clause)
            assert all(L != 0 for L in clause)

        # Accept the clauses as given, by default.
        self.clauses = [tuple(clause) for clause in clauses]

        # Is the CNF already unsatisfied?
        self.unsat = any(len(e) == 0 for e in self.clauses)

        # Save the comments, if any.
        self.comments = comments

        # Lazy statistics.
        self._max_variable = None
        self._num_variables = None
        self._num_literals = None
        self._width_stats = None

    def evaluate(self, assignment, return_offending_clause=False):
        """Evaluates the formula under the given assignment.

        The assignment should behave like a dictionary from variable
        indices to truth values (int -> bool), in terms of __getitem__
        and __contains__.
        """
        for i_clause, clause in enumerate(self.clauses):
            any_literal_true = False
            for literal in clause:
                variable = abs(literal)
                if variable in assignment:
                    if assignment[variable] == True:
                        if literal > 0:
                            any_literal_true = True
                            break
                        else:
                            continue
                    else:
                        if literal < 0:
                            any_literal_true = True
                            break
                        else:
                            continue
            if not any_literal_true:
                if return_offending_clause:
                    return i_clause
                else:
                    return False

        if return_offending_clause:
            return None
        else:
            return True

    def unsatisfied_clauses(self, assignment):
        """Returns indices of the clauses not satisfied by the assignment."""
        unsat_undef = []
        unsat_confl = []
        for i in range(len(self.clauses)):
            clause = self.clauses[i]
            any_literal_true = False
            any_literal_undef = False
            for literal in clause:
                variable = abs(literal)
                if variable in assignment:
                    if assignment[variable] == True:
                        if literal > 0:
                            any_literal_true = True
                            break
                        else:
                            continue
                    else:
                        if literal < 0:
                            any_literal_true = True
                            break
                        else:
                            continue
                else:
                    any_literal_undef = True
            if not any_literal_true:
                if any_literal_undef:
                    unsat_undef.append(i)
                else:
                    unsat_confl.append(i)
        return unsat_undef, unsat_confl

    def num_clauses(self):
        """Get the number of clauses."""
        return len(self.clauses)

    def max_variable(self):
        """Get the maximum variable (lazy)."""
        if self._max_variable is None:
            self._max_variable = 0
            for clause in self.clauses:
                for literal in clause:
                    self._max_variable = max(abs(literal), self._max_variable)
        return self._max_variable

    def num_variables(self):
        """Get the number of variables (lazy).
        
        Only the variables that actually occur in the problem are counted.
        """
        if self._num_variables is None:
            variables = set()
            for clause in self.clauses:
                for literal in clause:
                    variables.add(abs(literal))
            self._num_variables = len(variables)
        return self._num_variables

    def num_literals(self):
        """Get the total number of literals across all clauses (lazy)."""
        if self._num_literals is None:
            self._num_literals = sum(len(clause) for clause in self.clauses)
        return self._num_literals

    def width_stats(self):
        """Get clause width statistics (lazy)."""
        if self._width_stats is None:
            ws = defaultdict(lambda: 0)
            for clause in self.clauses:
                ws[len(clause)] += 1
            self._width_stats = dict(ws)
        return self._width_stats

    def shuffle(self, seed):
        """Returns another Cnf with the same structure but randomized:
          - order of clauses
          - variable names
          - variable signs
        """
        import random
        random.seed(seed)

        # Shuffle clauses.
        clause_order = list(range(len(self.clauses)))
        random.shuffle(clause_order)

        # Remap variables.
        variable_map = list(range(self.max_variable()))
        random.shuffle(variable_map)

        # Flip sign?
        flip_sign = [random.randint(0, 1) == 1 for i in range(self.max_variable())]

        clauses = []
        for i in clause_order:
            dst_clause = []
            src_clause = self.clauses[i]
            for L in src_clause:
                assert(L != 0)
                K = variable_map[abs(L) - 1] + 1
                if L < 0:
                    K = -K
                if flip_sign[abs(L) - 1]:
                    K = -K
                assert(K != 0)
                assert(abs(K) <= self.max_variable())
                dst_clause.append(K)
            clauses.append(tuple(dst_clause))

        return Cnf(clauses, self.comments)

    def to_file(self, f):
        """Writes as a DIMACS-format CNF problem."""
        if isinstance(f, str):
            if f.endswith('.gz'):
                with gzip.open(f, 'w') as f2:
                    return self.to_file(f2)
            else:
                with open(f, 'wb') as f2:
                    return self.to_file(f2)

        f.write('p cnf {} {}\n'.format(self.num_variables(), self.num_clauses()).encode('utf-8'))
        for clause in self.clauses:
            for L in clause:
                f.write('{} '.format(L).encode('utf-8'))
            f.write('0\n'.encode('utf-8'))

    @staticmethod
    def from_file(f):
        """Read a DIMACS-format CNF problem from the given text file.
        
        Returns an instance of class Cnf.
        """
        if isinstance(f, str):
            if f.endswith('.gz'):
                with gzip.open(f) as f2:
                    return Cnf.from_file(f2)
            else:
                with open(f) as f2:
                    return Cnf.from_file(f2)

        comments = []
        clauses = []
        n_vars = 0
        n_clauses = 0

        # Read the file line by line
        clause = []
        for i, line in enumerate(f):
            # Decode line if necessary
            if isinstance(line, bytes):
                line = line.decode('utf-8')

            # Handle comment line (may occur anywhere)
            if line.startswith('c'):
                comments.append(line[2:].rstrip())

            # Handle problem description line (exactly one of these allowed)
            elif line.startswith('p'):
                if n_vars > 0:
                    raise Exception('Duplicate instance description on line {}'.format(i))
                else:
                    problem_type, n_vars, n_clauses = line[2:].split()
                    if problem_type != 'cnf':
                        raise Exception('Not a cnf instance on line {}'.format(i))
                    n_vars = int(n_vars)
                    n_clauses = int(n_clauses)
                    if n_vars <= 0:
                        raise Exception('Bad number of variables on line {}'.format(i))
                    if n_clauses <= 0:
                        raise Exception('Bad number of clauses on line {}'.format(i))

            # Handle clause line (only allowed after the problem description)
            else:
                if n_vars == 0:
                    raise Exception('Unexpected line {}'.format(i))
                clause += [int(e) for e in line.split()]
                if len(clause) < 2:
                    raise Exception('Clause too short on line {}'.format(i))
                if clause[-1] == 0:
                    clauses.append(clause[:-1])
                    clause = []

        # Construct and return the Cnf object
        return Cnf(clauses, comments)

