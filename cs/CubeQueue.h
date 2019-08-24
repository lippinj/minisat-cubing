#ifndef CubeQueueH
#define CubeQueueH

#include <map>
#include <vector>
#include <unordered_map>

#include "Cube.h"

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
	int indexOf(const Cube&) const;

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
	std::unordered_map<Cube, std::pair<double, int>> implicants;
};

#endif
