/*********************************************************************************[Main_cubing.cc]
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

#include "minisat/core/SolverTypes.h"
#include "cs/CubifyingSolver.h"

Minisat::SimpSolver* makeSolver()
{
	return new Minisat::CubifyingSolver();
}

Minisat::lbool payload(Minisat::Solver* S)
{
	auto solver = dynamic_cast<Minisat::CubifyingSolver*>(S);
	assert(solver != nullptr);

	return solver->interleavedSolve();
}

void postSolve(Minisat::Solver* S, FILE* f)
{
	auto solver = dynamic_cast<Minisat::CubifyingSolver*>(S);
	solver->printStepStats();
	printf("final mean score      : %-12f\n", solver->meanScore());
}
