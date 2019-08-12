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

    // Enqueue a clause for cubification, unless it's too big.
	void pushCubify(const Cube&);

protected:
	lbool cubify(const Cube&);
	Cube cubifyInternal(const Cube&);

protected:
    // Queue of cubes to search on, ordered by the density score. This object
    // also tracks the mean density seen so far.
	CubeQueue cq;

    // Setlike index of all clauses that exist in the set of problem clauses
    // (i.e., clauses excluding learnts). The negations of the clauses, rather
    // than the clauses as such, are stored.
    CubeIndex ci;

    // Clauses enqueued for cubification and already cubified, both stored as
    // their negations.
	std::unordered_set<Cube> cubifyQueue;
	CubeIndex cubifyQueueDone;
};

} // namespace Minisat

#endif
