#ifndef CubifyingSolverBaseH
#define CubifyingSolverBaseH

#include <unordered_set>

#include "InterleavedSolver.h"
#include "Cube.h"

namespace Minisat
{
// Interleaved solver based on cubification: implicant cubes of the problem
// clauses are scored and roughly half of the search power is allocated to
// preferentially searching the branches that these cubes describe.
//
// (Implicant cubes are cubes containing all but one literal of the negation
// of some problem clause; they are so called because assuming an implicant
// trivially implies the negation of the last literal.)
//
// This is a base class. The inheriting class defines details such as which
// data structures are used and how.
//
// The bootstrap procedure is to enqueue every problem clause for
// cubification (defined in an inheriting class).
//
// The loop body is the following, marking the iteration budget as X:
//   1. Run a standard search for X conflicts
//   2. Mark as P the number of propagations in step 1
//   3. Spend k_c * P propagations on cubifying enqueued clauses, if any
//   4. Spend at most X conflicts on searching in the best-scored cubes
//   5. Simplify
// 
// In step 4, any cube reaching SAT also gives SAT for the full problem. Any
// cube reaching UNSAT gives some subclause of ~cube as an implicate clause
// of the formula. If the subclause is empty, the full problem is UNSAT.
//
// Configuration:
//  - Step 4 can be delayed until the cubification queue is empty.
//  - k_c can be adjusted to tune the time spend cubifying.
//  - override canCubify() to indicate if there are enqueued clauses.
//  - override cubifyOne() to define the cubification of one clause.
//  - override pickCube() to define which cube is next in line to search.
//  - override refuteCube() to define handling for a cube refuted in step 4.
class CubifyingSolverBase : public InterleavedSolver
{
public:
    CubifyingSolverBase() {}
    ~CubifyingSolverBase() {}

    virtual lbool interleavedSolveStep(double budget, int curr_restarts) override;

	// Push ~cube as a new clause. Does not check whether it exists already.
	bool learnNegationOf(const Cube& cube);

	void printStepStats() const;

public:
    // Multiplier that adjusts the time spend cubifying.
	double k_c = 2.0;

    // If set to false, step 4 only runs once cubifiable clauses have been
    // exhausted.
	bool alwaysSearchCube = true;

    // Counter: how many clauses have been cubified?
	uint64_t cubifications = 0;

    // Counter: how many cubes have been refuted?
	uint64_t cubeRefutations = 0;

protected:
    // Choose a cube to search on in step 4. Returns true if one was
    // available. The cube is received in the argument.
	virtual bool pickCube(Cube&);

    // Handle when the cube "base" was picked for search in step 4, and the
    // search indicated a conflict caused by the cube "reduced" (which is
    // necessarily either "base" or a subcube).
	virtual lbool refuteCube(const Cube& base, const Cube& reduced);

	virtual bool canCubify() const;
	virtual lbool cubifyOne();

protected:
	lbool searchCubeBranch(const Cube&, int budget);

	bool rootOf(const Clause&, Cube&);
	bool isConflicted(const Cube&);

	void dropClause(const int i);

protected:
	int exitPoint = 0;
	double stepTime0;
	double stepTime1;
	double stepTime2;
	double stepTime3;
	double stepTime4;
	double totalTimeSearch = 0.0;
	double totalTimeCubify = 0.0;
	double totalTimeSearchCube = 0.0;
	double totalTimeEndSimplify = 0.0;
};

} // namespace Minisat

#endif
