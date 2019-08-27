from .cnf import Cnf


def cnf_from_file(filename):
    """Read a DIMACS-format CNF problem from the given text file.
    
    Returns an instance of class Cnf.
    """
    comments = []
    clauses = []
    n_vars = 0
    n_clauses = 0

    # Read the file line by line
    clause = []
    with open(filename) as f:
        for i, line in enumerate(f):
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


def cnf_to_file(clauses, comments, filename):
    """Write the given CNF problem to a DIMACS-format file."""
    n_vars = max(max(abs(e) for e in c) for c in clauses)
    n_clauses = len(clauses)

    with open(filename, 'w') as f:
        for c in comments:
            f.write('c {}\n'.format(c))
        f.write('p cnf {} {}\n'.format(n_vars, n_clauses))
        for c in clauses:
            f.write(' '.join(str(e) for e in c) + ' 0\n')

