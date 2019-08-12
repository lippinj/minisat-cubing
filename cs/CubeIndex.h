#ifndef CubeIndexH
#define CubeIndexH

#include <unordered_map>
#include <unordered_set>
#include <memory>

#include "Cube.h"

class CubeIndex
{
public:
    CubeIndex();

	void push(const Cube&, size_t depth=0);
	void pop(const Cube&, size_t depth=0);

	bool contains(const Cube&, size_t depth=0) const;
	// bool containsOrSubsumes(const Cube&, size_t depth=0) const;

protected:
	// Child marks (if the mark is true, that cube exists).
    std::unique_ptr<std::unordered_set<int>> marks;

	// Child cube indices.
    std::unique_ptr<std::unordered_map<int, CubeIndex>> children;
};

// IMPLEMENTATION

inline CubeIndex::CubeIndex()
{
    marks.reset(new std::unordered_set<int>());
    children.reset(new std::unordered_map<int, CubeIndex>());
}

inline void CubeIndex::push(const Cube& cube, size_t depth)
{
#ifndef NO_CS_ASSERTS
	assert(depth < cube.size());
#endif

	auto x = cube[depth].x;
	if (cube.size() == (depth + 1)) {
		marks->insert(x);
	}
	else {
		(*children)[x].push(cube, depth + 1);
	}
}

inline void CubeIndex::pop(const Cube& cube, size_t depth)
{
#ifndef NO_CS_ASSERTS
	assert(depth < cube.size());
#endif

	auto x = cube[depth].x;
	if (cube.size() == (depth + 1)) {
		marks->erase(x);
	}
	else {
		(*children)[x].pop(cube, depth + 1);
	}
}

inline bool CubeIndex::contains(const Cube& cube, size_t depth) const
{
#ifndef NO_CS_ASSERTS
	assert(depth < cube.size());
#endif

	auto x = cube[depth].x;
	if (cube.size() == (depth + 1)) {
		return marks->count(x) > 0;
	}
	else {
		auto it = children->find(x);
		if (it == children->end()) return false;
		return it->second.contains(cube, depth + 1);
	}
}

#endif
