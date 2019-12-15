/***********************************************************************************[CubeQueue.cc]
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
#include "CubeQueue.h"

CubeQueue::CubeQueue(size_t budget) : budget(budget)
{
}

void CubeQueue::push(const Cube& cube, double score, int i)
{
    if (!contains(cube)) {
        if ((implicants.size() + 1) >= budget) {
            auto back = peekWorst();
            pop(back);
        }

        implicants[cube] = { score, { i } };
        scorewise[score].push_back(cube);

        sumScore += score;
        numSeen += 1.0;
    }
    else {
        addParentInd(cube, i);
    }
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

	for (auto j : v) {
		if (j == i) {
			return;
		}
	}

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
