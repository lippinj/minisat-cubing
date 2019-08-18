#ifndef BimapH
#define BimapH

#include <cassert>
#include <vector>
#include <unordered_map>

// Bidirectional int-to-int map.
//
// Used to associate each clause index (the index of clauses[i] is i) with a
// permanent index. (The transient index may change due to e.g. simplification
// steps.)
class Bimap
{
public:
	// Record a new clause:
	//   j = add(i)
	// where
	//   i is the current (transient) index;
	//   j is the permanent index (assigned by the object).
	int add(int i);

	// Drop the clause whose transient index is i.
	void drop(int i);

	// Swap the transient index i with transient index j.
	void swap(int i, int j);

	// Indicate that the clause with transient index i will get transient index j
	// at the next buffer flip.
	void will_move(int i, int j);

	// Enact a buffer flip.
	void flip_buffer();

	// Return the permanent index associated with the trasient index j.
	int fw(int j) const;

	// Return the transient index associated with the permanent index i.
	int bw(int i) const;

private:
	int next_free_index = 0;

	// Permanent-to-transient index map. Any clause that does not occur in this
	// map no longer exists in the set.
	std::unordered_map<int, int> ptt;

	// Transient-to-permanent index map.
	std::vector<int> ttp;

	// Transient-to-permanent index map (pending buffer flip).
	std::vector<int> ttp_next;
};

// Implementation below

inline int Bimap::add(int i)
{
#ifndef NO_CS_ASSERTS
	assert((ttp.size() <= i) || (ttp[i] == -1));
#endif

	int j = next_free_index++;

	ptt[j] = i;
	if (ttp.size() <= i) {
		ttp.resize(i + 1, -1);
	}
	ttp[i] = j;
	ttp[i] = j;

	return j;
}

inline void Bimap::drop(int i)
{
#ifndef NO_CS_ASSERTS
	assert(ttp.size() > i);
	assert(ttp[i] >= 0);
#endif

	ptt.erase(ttp[i]);
	ttp[i] = -1;
}

inline void Bimap::swap(int i, int j)
{
#ifndef NO_CS_ASSERTS
	assert(ttp.size() > i);
	assert(ttp.size() > j);
	assert(ttp[i] >= 0);
	assert(ttp[j] >= 0);
#endif

	auto ir = ttp[i];
	auto jr = ttp[j];
	ptt[ir] = j;
	ptt[jr] = i;
	ttp[i] = jr;
	ttp[j] = ir;
}

inline void Bimap::will_move(int i, int j)
{
#ifndef NO_CS_ASSERTS
	assert(ttp.size() > i);
	assert(ttp[i] >= 0);
#endif

	if (ttp_next.size() <= j) {
		ttp_next.resize(j + 1, -1);
	}
	ttp_next[j] = ttp[i];
}

inline void Bimap::flip_buffer()
{
	ttp.swap(ttp_next);
	ttp_next.clear();
	ptt.clear();
	for (int i = 0; i < ttp.size(); ++i) {
		ptt[ttp[i]] = i;
	}
}

inline int Bimap::fw(int j) const
{
	if (ptt.count(j) == 0) return -1;
	return ptt.at(j);
}

inline int Bimap::bw(int i) const
{
	if (ttp.size() <= i) return -1;
	return ttp[i];
}

#endif