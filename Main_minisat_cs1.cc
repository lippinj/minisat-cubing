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
