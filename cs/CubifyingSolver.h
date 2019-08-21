#ifndef CubifyingSolverH
#define CubifyingSolverH

#include <unordered_set>
#include <unordered_map>

#include "CubifyingSolverBase.h"
#include "CubeIndex.h"
#include "CubeQueue.h"
#include "Cube.h"

namespace Minisat
{
// Specific implementation of CubifyingSolverBase.
class CubifyingSolver : public CubifyingSolverBase
{
public:
	CubifyingSolver();
    ~CubifyingSolver();

	double meanScore() const;

public:
    // Only search inside cubes that are at least k_t times as dense as the
    // mean density seen so far.
	double k_t = 10.0;

    // Only cubify clauses of this size or smaller.
    int maxCubifiableSize = 6;

protected:
    // Pick the best cube in the queue, but only if it is dense enough (see
    // the k_t parameter above).
	virtual bool pickCube(Cube&) override;

    // Remove the base cube from the queue. If the negation of the reduced
    // cube is a new clause, learn it and pass it on as a cubification
    // candidate.
	virtual lbool refuteCube(const Cube&, const Cube&) override;

    // Can cubify if there are clauses in the queue.
	virtual bool canCubify() const override;

    // Dequeue and cubify() a single clause.
	virtual lbool cubifyOne() override;

    // Enqueue all problem clauses (as long as they are not too big).
	void bootstrap() override;

protected:
	// Cubify the clause with transient index i.
	lbool cubify(const int i);

	// Plan a path of propagate/cancel operations that touches every cube
	// implicant not already accounted for. Returns true by default; returns
	// false if an explicit subsumption was found.
	//
	//   In the path that is returned, a defined literal x means:
	//      1. propagate x, if not already implied
	//      2. enqueue implicant up to x
	//   An undefined literal (lit_Undef) means:
	//      1. cancel one level
	bool makeCubifyPathTrivial(const Cube&, std::vector<Minisat::Lit>&);

	// Returns a conflicting subcube of the root cube (in the typical case,
	// returns the cube by itself; the procedure may discover strengthenings,
	// however.)
	//
	// An exception is if the returned cube is empty, which indicates that the
	// cube is already subsumed in the problem and can be summarily dropped.
	Cube cubifyInternal(const int i, const Cube&);

	// Replace i:th clause with the negation of C.
	bool pruneClause(const int i, const Cube& C);

protected:
    // Queue of cubes to search on, ordered by the density score. This object
    // also tracks the mean density seen so far.
	CubeQueue cq;

    // Setlike index of all clauses that exist in the set of problem clauses
    // (i.e., clauses excluding learnts). The negations of the clauses, rather
    // than the clauses as such, are stored.
    CubeIndex ci;

	// Permanent indices of clauses to cubify.
	std::vector<int> cubifyQueue;
};

} // namespace Minisat

#endif
