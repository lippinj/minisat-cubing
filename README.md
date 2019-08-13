# minisat-cubing

This is an extension to the SAT solver
[Minisat](https://github.com/niklasso/minisat),
written by Niklas Een and Niklas Sörensson.
It interleaves the standard CDCL-like search of Minisat with preferential
search in branches specified by "implicant cubes".

This work is the implementation part of an MSc thesis.

## changes

The original Minisat code has been left unchanged as far as possible. A few
small changes were necessary to make compilation work also in VS.
