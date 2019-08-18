#include "minisat/utils/Options.h"
#include "minisat/utils/System.h"
#include "CubifyingSolver.h"

namespace Minisat
{
static const char* _cat = "CS";

static DoubleOption opt_k_t(_cat, "k_t",
        "Density threshold", 10.0, DoubleRange(0, false, HUGE_VAL, false));

static IntOption  opt_max_cubify(_cat, "max-cubify",
        "Maximum cubifiable size", 6, IntRange(2, INT_MAX));

static DoubleOption opt_k_c(_cat, "k_c",
        "Cubification coefficient", 10.0, DoubleRange(0, false, HUGE_VAL, false));

static BoolOption opt_always_search(_cat, "always-search",
        "Search inside a cube even before cubification is completed", false);

CubifyingSolver::CubifyingSolver()
{
    k_t = opt_k_t;
    k_c = opt_k_c;
    maxCubifiableSize = opt_max_cubify;
    alwaysSearchCube = opt_always_search;
}

CubifyingSolver::~CubifyingSolver()
{
}

bool CubifyingSolver::pickCube(Cube& cube)
{
	if (cq.empty()) {
		return false;
	}
	else if (cq.bestScore() < (k_t * cq.meanScore())) {
		return false;
	}
	else {
		cube = cq.peekBest(irand(random_seed, 1000000));
		return true;
	}
}

lbool CubifyingSolver::refuteCube(const Cube& base, const Cube& reduced)
{
	if (cq.contains(base)) cq.pop(base);

	if (!ci.contains(reduced)) {
		cubifyQueue.push_back(clauses.size());
		learnNegationOf(reduced);
		ci.push(reduced);
	}

	return ok ? l_Undef : l_False;
}

bool CubifyingSolver::pruneClause(const int i, const Cube& C)
{
	dropClause(i);
	if (!ci.contains(C)) {
		Minisat::vec<Lit> v;
		C.invert(v);
		return addClause_(v);
	}
}

void CubifyingSolver::bootstrap()
{
#ifndef NO_CS_ASSERTS
	assert(decisionLevel() == 0);
#endif

	cubifyQueue.reserve(clauses.size());
	for (int i = 0; i < clauses.size(); ++i) {
		cubifyQueue.push_back(bi.bw(i));
	}
}

bool CubifyingSolver::canCubify() const
{
	for (const auto j : cubifyQueue) {
		if (bi.fw(j) >= 0) {
			return true;
		}
	}
	return false;
}

lbool CubifyingSolver::cubifyOne()
{
	while (!cubifyQueue.empty()) {
		int j = cubifyQueue.back();
		cubifyQueue.pop_back();

		int i = bi.fw(j);
		if (i >= 0) {
			return cubify(i);
		}
	}
	return l_Undef;
}

lbool CubifyingSolver::cubify(const int i)
{
#ifndef NO_CS_ASSERTS
	assert(ok);
	assert(decisionLevel() == 0);
#endif

	CRef cr = clauses[i];
	const Clause& clause = ca[cr];

	// Reduce the clause to a minimal conflicting cube (minimal in a weak sense).
	Cube root;
	for (int k = 0; k < clause.size(); ++k) {
		auto L = clause[k];
		auto v = value(L);

		if (v == l_True) {
			// If the literal is true, the clause is satisfied.
			return l_Undef;
		}
		else if (v == l_False) {
			// If the literal is false, it need not be included.
		}
		else {
			// If the literal value is undefined, it is included.
			root.push(~L);
		}
	}

	// If the minimal conflict cube is too big, don't cubify it (but prune it, if
	// possible).
	if (root.size() > maxCubifiableSize) {
		if (root.size() < clause.size()) {
			return pruneClause(i, root) ? l_Undef : l_False;
		}
		return l_Undef;
	}

	// if (root.size() == 1) return refuteCube(root, root);

#ifndef NO_CS_ASSERTS
	// Sanity check.
	assert(ok);
	assert(root.size() > 1);
	assert(isConflicted(root));
#endif

	// Cubify the clause, unless it's unit under UP.
	//
	// If a subsumption is found, immediately replace the clause with
	// the reduced clause and enqueue the child for cubification.
	Cube post = cubifyInternal(root);

#ifndef NO_CS_ASSERTS
	// Sanity checks.
	assert(post.size() > 0);
	assert(post.subsetOf(root));
	assert(isConflicted(post));
#endif

	if (post.size() < clause.size())
	{
		dropClause(i);

		if (post.size() == 1)
		{
			const auto L = post[0];

#ifndef NO_CS_ASSERTS
			assert(value(L) == l_Undef);
#endif
			addClause(~L);
		}
		else {
			if (!ci.contains(post)) {
				cubifyQueue.push_back(clauses.size());

				vec<Lit> v;
				post.invert(v);
				addClause_(v);

				ci.push(post);
			}
		}
	}

	return ok ? l_Undef : l_False;
}

Cube CubifyingSolver::cubifyInternal(const Cube& root)
{
	int cubeSize = root.size() - 1;
	int level0 = decisionLevel();
	int trail0 = trail.size();

	Cube cube;

	// In the nominal case, there are cubeSize subcubes for clause: one for
	// each literal that can be dropped (from ~clause).
	for (int iSkip = cubeSize; iSkip >= 0; --iSkip)
	{
		// Loop invariant here:
		//
		// 1. For some n, the cube contains exactly the first n literals of
		//    the subcube skipping i (n may be 0).
		//
		// 2. The literals in the cube correspond to current decision levels
		//    beyond the zeroth one.
#ifndef NO_CS_ASSERTS
		assert(cube.size() <= iSkip);
		assert((decisionLevel() - level0) == cube.size());
		assert(root.startsWith(cube));
#endif

		// Has the cube skipping i already been processed somewhere?
		Cube term;
		for (int i = 0; i < root.size(); ++i) {
			if (i != iSkip) {
				term.push(root[i]);
			}
		}

		// Only process the cube explicitly if it has not.
		if (!ci.contains(term) && !cq.contains(term))
		{
			bool conflictFound = false;
			for (int i = 0; i < root.size(); ++i)
			{
				if (i < cube.size()) continue;
				if (i == iSkip) continue;

				Lit L = root[i];
				lbool v = value(L);

				newDecisionLevel();
				if (v != l_True)
				{
					cube.push(L);

					if (ci.contains(cube)) {
						conflictFound = true;
						break;
					}

					if (v == l_False) {
						conflictFound = true;
						break;
					}
					else {
						enqueue(L);
						if (propagate() != CRef_Undef) {
							conflictFound = true;
							break;
						}
					}
				}

				double num = trail.size() - trail0;
				double den = cube.size();
				double score = num / den;
				cq.push(cube, score);
			}

			if (conflictFound) {
				cancelUntil(level0);
				return cube;
			}
		}

		// Unwind down to only those literals that are also shared with the
		// next subcube.
		if (iSkip == 0) {
			cancelUntil(level0);
		}
		else {
			int iSkipNext = iSkip - 1;
			cancelUntil(level0 + iSkipNext);

			cube.clear();
			int keepSize = decisionLevel() - level0;
			for (int i = 0; i < keepSize; ++i) {
				cube.push(root[i]);
			}
		}
	}

	// If this point is reached, no subcube caused a conflict.
	return root;
}

double CubifyingSolver::meanScore() const
{
	return cq.meanScore();
}

} // namespace Minisat
