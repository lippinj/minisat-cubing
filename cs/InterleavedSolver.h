#ifndef InterleavedSolverH
#define InterleavedSolverH

#include "minisat/simp/SimpSolver.h"

namespace Minisat
{
// This class generalizes the solving procedure of SimpSolver (in the
//no-assumptions case). The original procedure is as follows:
//
//   1. Run elimination (unless disabled)
//   2. Solver loop:
//       2a. Determine iteration budget based on restart policy
//       2b. CDCL search with current iteration budget (search())
//       2c. If solved or out of overall budget, break the loop
//   3. If SAT, extend the model
//
// The interleaved solver allows the following:
//   - define arbitrary code to run between steps 1 and 2 (bootstrap());
//   - replace step 2b with arbitrary code (interleavedSolveStep()).
//
// The name "interleaved" refers to the use case of doing something in
// addition to the search() call, rather than instead of it. The result is
// that the search() calls are "interleaved" with this other code.
class InterleavedSolver : public SimpSolver
{
public:
    InterleavedSolver() {}
    ~InterleavedSolver() {}

    // Call this instead of solve() in order to use the interleaved procedure
    lbool interleavedSolve();

protected:
    // This function is executed before the solver loop.
    virtual void bootstrap();

    // This function is executed as the 2b step at every loop iteration.
    // Returns:
    //   l_Undef if a conclusion could not be reached
    //   l_True  if a model has been found
    //   l_False if unsatisfiability has been established
    virtual lbool interleavedSolveStep(double budget, int curr_restarts);

    virtual void printStatTableHead() const;
    virtual void printStatTableEnd() const;

protected:
    lbool interleavedSolveInternal();

    // Returns the x:th element of the Luby sequence.
    int luby(int x) const;

    // Returns base to the power of luby(x).
    double lubyExp(int x, double base) const;

    int toDimacsInt(Lit L) const;

	void checkSane();
};

} // namespace Minisat

#endif
