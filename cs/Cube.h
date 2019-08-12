#ifndef CubeH
#define CubeH

#include <vector>
#include "minisat/core/SolverTypes.h"
using Minisat::Lit;

struct Cube {
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
    void invert(Minisat::vec<Lit>&) const;

    Lit operator[](size_t) const;
    const Lit* begin() const;
    const Lit* end() const;
	bool contains(const Lit) const;
    bool subsetOf(const Cube&) const;
    bool startsWith(const Cube&) const;
	bool sane() const;

    size_t hash() const;

public:
	static Cube inverted(const Minisat::Clause& clause);

private:
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
                literals[i] = literals[i+1];
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
