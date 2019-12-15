/************************************************************************************[CubeIndex.h]
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

#ifndef CubeIndexH
#define CubeIndexH

#include <unordered_map>
#include <unordered_set>
#include <memory>

#include "Cube.h"

// Intended as a compact and fast set implementation for cubes.
class CubeIndex
{
public:
    CubeIndex();

	// Add/remove a cube to the set. Callers from outside should ignore the
	// depth argument.
	void push(const Cube&, size_t depth=0);
	void pop(const Cube&, size_t depth=0);

	bool contains(const Cube&, size_t depth=0) const;

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
