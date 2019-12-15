/************************************************************************************[CubeQueue.h]
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

#ifndef CubeQueueH
#define CubeQueueH

#include <map>
#include <vector>
#include <unordered_map>

#include "Cube.h"

// A queue of cubes under which we can search.
//
// Essentially, this is a mapping of:
//    cube -> score, [clause_ids...]
// such that the cube with the highest score is quickly accessible.
//
// The clause_ids are persistent indices (see Bimap.h) for clauses that are
// known to be subsumed by the negation of the cube. In other words, if the
// cube gets refuted, any clauses in clause_ids that still exist can be
// dropped and replaced with one count of the negation of the cube.
class CubeQueue
{
public:
	CubeQueue(size_t cubeBudget=1000000);

	// Register a cube with the given score.
	void push(const Cube& cube, double score, int i);

	// Remove the given cube from the queue.
	void pop(const Cube&);

	// Returns the best cube in the queue.
	const Cube peekBest(int rnd=0) const;

	// Returns the best cube in the queue.
	const Cube peekWorst() const;

	// Is the given cube recorded here, as a conflict?
	bool contains(const Cube&) const;

	// Persistent index of the parent clause.
	void addParentInd(const Cube&, int i);
	std::vector<int> getParentInds(const Cube&) const;

	// Is the queue empty?
	bool empty() const;

	// How many cubes are in the queue?
	size_t size() const;

	// Best score in the queue.
	double bestScore() const;

	// Mean score seen so far.
	double meanScore() const;

protected:
    size_t budget;
	double sumScore = 0.0;
	double numSeen = 0.0;

	// Map like: cubeScore -> [cubes...]
	std::map<double, std::vector<Cube>> scorewise;

	// Map like: cube -> (cubeScore, persistentParentIndex)
	std::unordered_map<Cube, std::pair<double, std::vector<int>>> implicants;
};

#endif
