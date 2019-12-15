/*****************************************************************************************[Cube.h]
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

#ifndef CubeH
#define CubeH

#include <vector>
#include "minisat/core/SolverTypes.h"
using Minisat::Lit;

// A conjunction of literals.
struct Cube
{
	Cube();
	Cube(std::initializer_list<Lit>);
    ~Cube();

	size_t size() const;

    void clear();
    void push(Lit);
    void pop(Lit);

    bool operator==(const Cube& other) const;
    bool operator!=(const Cube& other) const;
	bool operator<(const Cube& other) const;
	Cube operator+(const Cube& other) const;

    Lit operator[](size_t) const;
    const Lit* begin() const;
    const Lit* end() const;
	bool contains(const Lit) const;
    bool subsetOf(const Cube&) const;
    bool startsWith(const Cube&) const;
	bool sane() const;

    size_t hash() const;

public:
	// Populates v with the negation of this cube.
    void invert(Minisat::vec<Lit>& v) const;

	// Returns the cube that is the negation of C.
	static Cube inverted(const Minisat::Clause& C);

private:
	// Literals in this clause. This is kept in sorted order.
	std::vector<Lit> literals;
};

// specialization for std::hash
inline size_t Cube::hash() const
{
    size_t x = 0;
    for (auto L : literals) {
        x = (x << 27) | (x >> (sizeof(size_t) * 8 - 19));
		x ^= Minisat::toInt(L);
    }
    return x;
}

namespace std {
    template<> struct hash<Cube> {
        size_t operator()(const Cube& cube) const { return cube.hash(); }
    };
}

inline Cube::Cube()
{
}

inline Cube::Cube(std::initializer_list<Lit> il)
{
	for (auto L : il) push(L);
}

inline Cube::~Cube()
{
}

inline size_t Cube::size() const
{
	return literals.size();
}

inline void Cube::clear()
{
    literals.clear();
}

inline void Cube::push(Lit L)
{
    // If the literal is already contained, do nothing.
    if (contains(L)) return;

    // Bubble insert the literal.
    const size_t N = literals.size();
    literals.push_back(L);
    for (size_t i = N; i > 0; --i)
    {
        if (literals[i] < literals[i - 1]) {
            std::swap(literals[i-1], literals[i]);
        }
        else {
            return;
        }
    }
}

inline void Cube::pop(Lit L)
{
    size_t i = 0;
    const size_t N = literals.size();

    while (i < N)
    {
        if (literals[i] == L)
        {
            while (i < (N - 1))
            {
                literals[i] = literals[i + 1];
				++i;
            }
            literals.resize(N - 1);
            return;
        }
        ++i;
    }
}

inline bool Cube::operator==(const Cube& other) const
{
    return literals == other.literals;
}

inline bool Cube::operator!=(const Cube& other) const
{
    return literals != other.literals;
}

inline bool Cube::operator<(const Cube& other) const
{
	return literals < other.literals;
}

inline Cube Cube::operator+(const Cube& other) const
{
    Cube ret;
    ret.literals = literals;
    for (size_t i = 0; i < other.literals.size(); ++i) {
        ret.push(other.literals[i]);
    }
    return ret;
}

inline void Cube::invert(Minisat::vec<Lit>& clause) const
{
    for (size_t i = 0; i < literals.size(); ++i) {
		clause.push(~literals[i]);
	}
}

inline Lit Cube::operator[](size_t n) const
{
    return literals[n];
}

inline const Lit* Cube::begin() const
{
    return literals.data();
}

inline const Lit* Cube::end() const
{
    return literals.data() + literals.size();
}

inline bool Cube::contains(const Minisat::Lit L) const
{
    for (size_t i = 0; i < literals.size(); ++i) {
        if (L == literals[i]) return true;
    }
    return false;
}

inline bool Cube::subsetOf(const Cube& other) const
{
    for (size_t i = 0; i < literals.size(); ++i) {
        if (!other.contains(literals[i])) {
            return false;
        }
    }
    return true;
}

inline bool Cube::startsWith(const Cube& other) const
{
    if (other.size() > size()) return false;
    for (int i = 0; i < other.size(); ++i) {
        if (other[i] != literals[i]) return false;
    }
    return true;
}

inline bool Cube::sane() const
{
    for (size_t i = 0; i < literals.size() - 1; ++i) {
		auto A = literals[i];
		auto B = literals[i + 1];
		if (!(A < B)) return false;
		if (var(A) == var(B)) return false;
    }
	return true;
}

inline Cube Cube::inverted(const Minisat::Clause& clause)
{
	Cube cube;
	for (size_t i = 0; i < clause.size(); ++i) {
		cube.push(~clause[i]);
	}
	return cube;
}

#endif
