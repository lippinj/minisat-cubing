# minisat-cubing

This is an extension to the SAT solver
[Minisat](https://github.com/niklasso/minisat),
written by Niklas Een and Niklas Sörensson.
It interleaves the standard CDCL-like search of Minisat with preferential
search in branches specified by "implicant cubes".

This work is the implementation part of an MSc thesis.

## changes

The `Main.cc` file is a modification of `simp/Main.cc` of MiniSat.

Additionally, the following files from MiniSat have been changed:
`minisat/core/Solver.h`,
`minisat/core/Solver.cc`, 
`minisat/utils/System.cc`.
