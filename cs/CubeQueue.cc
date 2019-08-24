#include <algorithm>
#include "CubeQueue.h"

CubeQueue::CubeQueue(size_t budget) : budget(budget)
{
}

void CubeQueue::push(const Cube& cube, double score, int i)
{
	if (contains(cube)) return;

    if ((implicants.size() + 1) >= budget) {
        auto back = peekWorst();
        pop(back);
    }

	implicants[cube] = { score, { i } };
	scorewise[score].push_back(cube);

	sumScore += score;
	numSeen += 1.0;
}

void CubeQueue::pop(const Cube& cube)
{
#ifndef NO_CS_ASSERTS
	assert(contains(cube));
#endif

	double score = implicants[cube].first;
	auto& vec = scorewise[score];
	if (vec.size() == 1) {
		scorewise.erase(score);
	}
	else {
		auto it = std::find(vec.begin(), vec.end(), cube);
		vec.erase(it);
	}
	implicants.erase(cube);
}

const Cube CubeQueue::peekBest(int r) const
{
	auto it = scorewise.rbegin();
	const auto& vec = it->second;
    if (vec.size() == 1) return vec[0];
    else return vec[r % vec.size()];
}

const Cube CubeQueue::peekWorst() const
{
	auto it = scorewise.begin();
	const auto& vec = it->second;
	return vec.front();
}

bool CubeQueue::contains(const Cube& cube) const
{
	return implicants.find(cube) != implicants.end();
}

void CubeQueue::addParentInd(const Cube& cube, int i)
{
	auto& v = implicants.at(cube).second;

#ifndef NO_CS_ASSERTS
	for (auto j : v) {
		assert(j != i);
	}
#endif

	v.push_back(i);
}

std::vector<int> CubeQueue::getParentInds(const Cube& cube) const
{
	return implicants.at(cube).second;
}

bool CubeQueue::empty() const
{
	return implicants.size() == 0;
}

size_t CubeQueue::size() const
{
	return implicants.size();
}

double CubeQueue::bestScore() const
{
	if (empty()) return 0.0;
	else return scorewise.rbegin()->first;
}

double CubeQueue::meanScore() const
{
	return sumScore / numSeen;
}
