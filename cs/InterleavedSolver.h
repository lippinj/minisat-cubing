/****************************************************************************[InterleavedSolver.h]
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
