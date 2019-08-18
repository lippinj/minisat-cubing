#ifndef ClauseIndexH
#define ClauseIndexH

#include <unordered_map>

#include "minisat/core/Solver.h"
#include "minisat/mtl/Vec.h"

// Index for checking whether or not a clause exists in the primary clause set.
class ClauseIndex
{
public:
	bool contains(const Minisat::Vec<Minisat::Lit>& clause) const;

private:
	Minisat::Solver* s;

	std::unordered_multimap<int, 
};

#endif
