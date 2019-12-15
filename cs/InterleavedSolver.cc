/***************************************************************************[InterleavedSolver.cc]
Copyright (c) 2019, Joonas Lipping

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include <algorithm>
#include "InterleavedSolver.h"

namespace Minisat
{

lbool InterleavedSolver::interleavedSolve()
{
    vec<Var> extra_frozen;
    lbool result = l_True;

    if (use_simplification) {
        result = lbool(eliminate(false));
    }

    if (result == l_True) {
        result = interleavedSolveInternal();
    }
    else if (verbosity >= 1) {
        printStatTableEnd();
    }

    if (result == l_True && extend_model) {
        extendModel();
    }

    return result;
}

void InterleavedSolver::bootstrap()
{
}

lbool InterleavedSolver::interleavedSolveStep(double budget, int curr_restarts)
{
    // Default solver step: just search from the top.
    int nof_conflicts = int(budget);
    return search(nof_conflicts);
}

void InterleavedSolver::printStatTableHead() const
{
    printf("============================[ Search Statistics ]==============================\n");
    printf("| Conflicts |          ORIGINAL         |          LEARNT          | Progress |\n");
    printf("|           |    Vars  Clauses Literals |    Limit  Clauses Lit/Cl |          |\n");
    printf("===============================================================================\n");
}

void InterleavedSolver::printStatTableEnd() const
{
    printf("===============================================================================\n");
}

lbool InterleavedSolver::interleavedSolveInternal()
{
    // Clear internals.
    model.clear();
    conflict.clear();

    // If we know we're UNSAT already, there is nothing to do.
    if (!ok) return l_False;

    // Set the initial learnt clause budget, as well as the schedule
    // for increasing it.
    max_learnts = std::max(int(nClauses() * learntsize_factor), min_learnts_lim);
    learntsize_adjust_confl = learntsize_adjust_start_confl;
    learntsize_adjust_cnt = (int)learntsize_adjust_confl;

    // Print the head of the solving statistics table.
    if (verbosity >= 1) printStatTableHead();

	// Preparations that need to run before any cycles do.
	bootstrap();

    // Run the solver under the current restart policy.
    lbool status = l_Undef;
    for (int curr_restarts = 0; status == l_Undef; curr_restarts++) {
        // Find the budget for this solver run.
        double budget;
        if (luby_restart) {
            budget = lubyExp(curr_restarts, restart_inc);
        }
        else {
            budget = pow(curr_restarts, restart_inc);
        }
		budget *= restart_first;

        // Execute a solver run under this budget.
        status = interleavedSolveStep(budget, curr_restarts);

        // Check for early exit conditions:
        //  - interrupted by SIGINT
        //  - out of allowed conflicts
        //  - out of allowed propagations
        if (!withinBudget()) break;
    }

    // Print the end of the solving statistics table.
    if (verbosity >= 1) printStatTableEnd();

    // If solving succeeded, update the internal state:
    //  - If a satisfying assignment was found, extend the model.
    //  - If unsatisfiability was detected, mark us UNSAT.
    if (status == l_True) {
        model.growTo(nVars());
        for (int i = 0; i < nVars(); i++) model[i] = value(i);
    }
    else if (status == l_False && conflict.size() == 0) {
        ok = false;
    }

    cancelUntil(0);
    return status;
}

int InterleavedSolver::luby(int x) const
{
    int size = 1;
    int seq = 0;
    while (size < (x + 1)) {
        seq++;
        size = (2 * size) + 1;
    }

    while (size-1 != x) {
        size = (size-1)>>1;
        seq--;
        x = x % size;
    }

    return seq;
}

double InterleavedSolver::lubyExp(int x, double base) const
{
    auto t = luby(x);
    return pow(base, t);
}

int InterleavedSolver::toDimacsInt(Lit L) const
{
    return (sign(L) ? -1 : 1) * (var(L) + 1);
}

void InterleavedSolver::checkSane()
{
	for (int i = 0; i < clauses.size(); ++i) {
		Clause& c = ca[clauses[i]];
		if (satisfied(c)) continue;
		assert(value(c[0]) == l_Undef && value(c[1]) == l_Undef);
	}
	for (int i = 0; i < learnts.size(); ++i) {
		Clause& c = ca[learnts[i]];
		if (satisfied(c)) continue;
		assert(value(c[0]) == l_Undef && value(c[1]) == l_Undef);
	}
}

} // namespace Minisat
