/*************************************************************************[CubifyingSolverBase.cc]
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
#include "minisat/utils/System.h"
#include "CubifyingSolverBase.h"

namespace Minisat
{
lbool CubifyingSolverBase::interleavedSolveStep(double budget, int curr_restarts)
{
	lbool status = l_Undef;

	// The search steps use the budget value as their conflict budget.
	// The cubification is time-limited to last as long as the previous
	// search step.
	int conflictBudget = int(budget);

	// STEP: default search
	//
	// Search without assumptions.
	stepTime0 = cpuTime();
	auto propagationsBeforeSearch = propagations;
	status = search(conflictBudget);
	stepTime1 = cpuTime();
	totalTimeSearch += (stepTime1 - stepTime0);

	// STEP: cubification
	//
	// Find and score the implicant cubes of one or more clauses. Continue
	// cubifying until at least as many propagations have been used as were used
	// by the default search step. Any clause that enters cubification is
	// cubified in full.
	if (status == l_Undef) {
		auto propagationBudget = k_c * (propagations - propagationsBeforeSearch);
		int propagationLimit = propagations + propagationBudget;

		while (propagations < propagationLimit) {
			if (!withinBudget()) break;
			if (!canCubify()) break;

			cubifications++;
			status = cubifyOne();
			if (status != l_Undef) {
				exitPoint = 1;
				break;
			}
		}
	}
	stepTime2 = cpuTime();
	totalTimeCubify += (stepTime2 - stepTime1);

	// STEP: cube search
	//
	// Search while assuming the topmost admissible cube, for as long as the
	// budget allows. If alwaysSearchCube is false, this step only executes
	// once all cubes have been scored.
	if ((status == l_Undef) && (!canCubify() || alwaysSearchCube)) {
		Cube cube;
		int conflicts_limit = conflicts + conflictBudget;
		while (conflicts < conflicts_limit)
		{
			if (!withinBudget()) break;
			if (!pickCube(cube)) break;

#ifndef NO_CS_ASSERTS
			assert(cube.size() > 0);
#endif

			status = searchCubeBranch(cube, conflicts_limit - conflicts);

			if (status == l_True) {
				exitPoint = 2;
                break;
			}
			else if (status == l_False) {
				cubeRefutations++;

				if (conflict.size() == 0) {
					exitPoint = 4;
                    break;
				}
				else {
					Cube reduced;
					for (int i = 0; i < conflict.size(); ++i) {
						reduced.push(~conflict[i]);
					}
#ifndef NO_CS_ASSERTS
					assert(reduced.subsetOf(cube));
#endif

					status = refuteCube(cube, reduced);
					if (status == l_False) {
						exitPoint = 3;
						break;
					}
				}
			}

#ifndef NO_CS_ASSERTS
			assert(status == l_Undef);
#endif
		}
	}
	stepTime3 = cpuTime();
	totalTimeSearchCube += (stepTime3 - stepTime2);

	if ((status == l_Undef) && !simplify()) {
		exitPoint = 5;
		return l_False;
	}
	stepTime4 = cpuTime();
	totalTimeEndSimplify += (stepTime4 - stepTime3);

	return status;
}

lbool CubifyingSolverBase::searchCubeBranch(const Cube& cube, int budget)
{
#ifndef NO_CS_ASSERTS
	assert(cube.sane());
	assert(decisionLevel() == 0);
	assert(assumptions.size() == 0);
#endif

	for (auto it = cube.begin(); it != cube.end(); ++it) {
		assumptions.push(*it);
	}

	conflict.clear();
	auto status = search(budget);

	if (status == l_True) return l_True;

	cancelUntil(0);
	assumptions.clear();

	return status;
}

bool CubifyingSolverBase::rootOf(const Clause& clause, Cube& cube)
{
	for (size_t j = 0; j < clause.size(); ++j)
	{
		Minisat::Lit L = clause[j];
		if (value(L) == Minisat::l_True) {
			return true;
		}
		else if (value(L) == Minisat::l_Undef) {
			cube.push(~L);
		}
	}
	return false;
}

bool CubifyingSolverBase::isConflicted(const Cube& cube)
{
#ifndef NO_CS_ASSERTS
	assert(decisionLevel() == 0);
#endif

	newDecisionLevel();
	for (auto it = cube.begin(); it != cube.end(); ++it) {
		auto L = *it;
		if (value(L) == l_False) {
			cancelUntil(0);
			return true;
		}
		else {
			enqueue(L);
		}
	}
	if (propagate() != CRef_Undef) {
		cancelUntil(0);
		return true;
	}

	cancelUntil(0);
	return false;
}

bool CubifyingSolverBase::pickCube(Cube&)
{
	return false;
}

lbool CubifyingSolverBase::refuteCube(const Cube& base, const Cube& reduced)
//lbool CubifyingSolverBase::refuteCube(const Cube& base)
{
	learnNegationOf(base);
	return ok ? l_Undef : l_False;
}

bool CubifyingSolverBase::canCubify() const
{
	return false;
}

lbool CubifyingSolverBase::cubifyOne()
{
	return l_Undef;
}

bool CubifyingSolverBase::learnNegationOf(const Cube& cube)
{
#ifndef NO_CS_ASSERTS
	assert(cube.size() > 0);
	assert(decisionLevel() == 0);
#endif

	Minisat::vec<Lit> v;
	cube.invert(v);
	return addClause_(v);
}

void CubifyingSolverBase::dropClause(const int i)
{
#ifndef NO_CS_ASSERTS
	assert(i < clauses.size());
#endif

	const int j = clauses.size() - 1;
	if (i == j) {
		bi.drop(j);
		removeClause(clauses[j]);
		clauses.shrink(1);
	}
	else {
		const CRef cri = clauses[i];
		const CRef crj = clauses[j];
		clauses[i] = crj;
		clauses.shrink(1);
		bi.swap(i, j);
		bi.drop(j);
		removeClause(cri);
	}
}

void CubifyingSolverBase::printStepStats() const
{
	double totalTime
		= totalTimeSearch
		+ totalTimeCubify
		+ totalTimeSearchCube;

	double pctTimeSearch = 100.0 * totalTimeSearch / totalTime;
	double pctTimeCubify = 100.0 * totalTimeCubify / totalTime;
	double pctTimeSearchCube = 100.0 * totalTimeSearchCube / totalTime;
	double pctTimeEndSimplify = 100.0 * totalTimeEndSimplify / totalTime;

	printf("| Search:       %12.2f s (%5.2f %%)\n", totalTimeSearch, pctTimeSearch);
	printf("| Cubification: %12.2f s (%5.2f %%)\n", totalTimeCubify, pctTimeCubify);
	printf("| Search(cube): %12.2f s (%5.2f %%)\n", totalTimeSearchCube, pctTimeSearchCube);
	printf("| End simplify: %12.2f s (%5.2f %%)\n", totalTimeEndSimplify, pctTimeEndSimplify);
	printf("| Exit:         %12d\n", exitPoint);
	printf("===============================================================================\n");
	printf("cubifications         : %-12ld\n", cubifications);
	printf("cube refutations      : %-12ld\n", cubeRefutations);
}

} // namespace Minisat
