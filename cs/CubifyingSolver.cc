/*****************************************************************************[CubifyingSolver.cc]
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

#include "minisat/utils/Options.h"
#include "minisat/utils/System.h"
#include "CubifyingSolver.h"

namespace Minisat
{
static const char* _cat = "CS";

static DoubleOption opt_k_t(_cat, "k_t",
        "Density threshold", 5.0, DoubleRange(0, false, HUGE_VAL, false));

static IntOption  opt_max_cubify(_cat, "max-cubify",
        "Maximum cubifiable size", 6, IntRange(2, INT_MAX));

static DoubleOption opt_k_c(_cat, "k_c",
        "Cubification coefficient", 1.0, DoubleRange(0, false, HUGE_VAL, false));

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
#ifndef NO_CS_ASSERTS
	assert(cq.contains(base));
#endif

	auto ii = cq.getParentInds(base);
	cq.pop(base);

	for (auto i : ii) {
		int j = bi.fw(i);
		if (j >= 0) {
			dropClause(j);
		}
	}

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

	literalDifficulty.resize(2 * nVars(), INT_MAX);
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
	Cube post = cubifyInternal(i, root);

	// Special case: an empty postcube indicates that the clause under inspection
	// is subsumed by another problem clause.
	if (post.size() == 0) {
		dropClause(i);
		return ok ? l_Undef : l_False;
	}

#ifndef NO_CS_ASSERTS
	// Sanity checks.
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

bool CubifyingSolver::makeCubifyPathBasic(const Cube& C, std::vector<Lit>& path)
{
	std::vector<Lit> c(C.begin(), C.end());
	return makeCubifyPath(c, path);
}

bool CubifyingSolver::makeCubifyPathDifficultyOrder(const Cube& C, std::vector<Lit>& path, int iClause)
{
	// First of all, place in front every literals L such that we already have
	// a score for C \ L.
	std::vector<Lit> c(C.size());
	int iNextSkippable = 0;
	int iNextNormal = C.size() - 1;

	for (const auto L : C) {
		Cube term;
		for (const auto K : C) {
			if (K != L) {
				term.push(K);
			}
		}

		if (cq.contains(term)) {
			cq.addParentInd(term, bi.bw(iClause));
			c[iNextSkippable++] = L;
		}
		else {
			c[iNextNormal--] = L;
		}
	}

	// Second, order the rest of the literals from predicted-hardest to
	// predicted-easiest.
	auto it = c.begin() + iNextSkippable;
	std::sort(it, c.end(), [&](const Lit& lhs, const Lit& rhs) {
			return literalDifficulty[lhs.x] > literalDifficulty[rhs.x]; });

	return makeCubifyPath(c, path);
}

bool CubifyingSolver::makeCubifyPath(const std::vector<Lit>& C, std::vector<Lit>& path)
{
	int N = C.size();
	Cube cube;

	// Each i corresponds to the subcube C \ C[i], which is of size N - 1.
	//
	// At each i, cube is some prefix of C, with size of at most i, e.g.:
    //   cube = []
    //   cube = C[0]
    //   ...
    //   cube = C[0,...,i-1]
	//
	// We reach C \ C[i] by pushing the remaining literals, except for C[i].
	//
	// The path and its cancellation are pushed to the stack, except if we
	// already know a score for C \ C[i], in which case that i is skipped.
	for (int i = 0; i < N; ++i)
	{
		Cube terminal;
		for (int j = 0; j < N; ++j) {
			if (j != i) {
				terminal.push(C[j]);
			}
		}

		if (!cq.contains(terminal))
		{
			for (int j = 0; j < N; ++j)
			{
				if (j == i) continue;
                if ((j < i) && (j < cube.size())) continue;
                if ((j > i) && (j <= cube.size())) continue;

				auto L = C[j];

				cube.push(L);
				if (ci.contains(cube)) {
					return false;
				}

				path.push_back(L);
			}

			for (int j = i + 1; j < N; ++j) {
				cube.pop(C[j]);
				path.push_back(lit_Undef);
			}
		}
	}

	return true;
}

Cube CubifyingSolver::cubifyInternal(const int i, const Cube& root)
{
	std::vector<Lit> path;
	auto pathOk = makeCubifyPathDifficultyOrder(root, path, i);
	if (!pathOk) return Cube();

	int level0 = decisionLevel();
	int trail0 = trail.size();

	Cube cube;
	bool conflict = false;
	std::vector<Lit> stack;
	for (const auto L : path)
	{
		// Pop top literal
		if (L == lit_Undef) {
			cube.pop(stack.back());
			stack.pop_back();
			cancelUntil(decisionLevel() - 1);
		}

		// Push literal L
		else {
			stack.push_back(L);
			newDecisionLevel();
			auto v = value(L);

			// Case 1:
			// L is in UP-conflict with the current state.
			// Subsuming clause ~(C u L) found; exit.
			if (v == l_False) {
				cube.push(L);
				conflict = true;
				break;
			}
			//  Case 2:
			// L is UP-required by the current state.
			// Enqueuing and propagating it would be tautological.
			else if (v == l_True) {
				continue;
			}
			// Case 3:
			// Enqueue and propagate L. Record the score for (C u L).
			// 
			// Exception: if propagation shows that (C u L) is a conflict,
			// we found a subsuming clause ~(C u L).
			else {
				auto propagationsBefore = propagations;

				cube.push(L);
				enqueue(L);
				if (propagate() != CRef_Undef) {
					conflict = true;
					break;
				}

				if (cube.size() == 1) {
					literalDifficulty[L.x] = propagations - propagationsBefore;
				}

				double num = trail.size() - trail0;
				double den = cube.size();
				double score = num / den;
                if (score > 1.0) {
                    cq.push(cube, score, bi.bw(i));
                }
			}
		}
	}

	// Done going through the path; unwind the stack.
	cancelUntil(level0);

	// If a subsumption was found, return it.
	if (conflict) return cube;

	// Otherwise, return the original cube.
	return root;
}

double CubifyingSolver::meanScore() const
{
	return cq.meanScore();
}

} // namespace Minisat
