CXX=g++

CXX_STD=-std=c++14
#CXX_STD=
CXX_OPT=-O3 -D NO_CS_ASSERTS
#CXX_OPT=-O3
CXX_DBG=-D NDEBUG
#CXX_OPT=
#CXX_DBG=-g -O0 -D DEBUG
CXXFLAGS=$(CXX_STD) $(CXX_OPT) $(CXX_DBG)\
		 -D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS\
		 -I.\
		 -Wno-literal-suffix

BUILDDIRS=\
		  build\
		  build/minisat\
		  build/minisat/core\
		  build/minisat/simp\
		  build/minisat/utils\
		  build/cs

OBJECTS=\
		build/minisat/core/Solver.o\
		build/minisat/simp/SimpSolver.o\
		build/minisat/utils/System.o\
		build/minisat/utils/Options.o\
		build/cs/ClauseIndex.o\
		build/cs/CubeQueue.o\
		build/cs/InterleavedSolver.o\
		build/cs/CubifyingSolverBase.o\
		build/cs/CubifyingSolver.o\
		build/Main.o\
		build/Main_cubing.o

all: $(OBJECTS)
	rm -f minisat_cubing
	$(CXX) $(CXXFLAGS) $(OBJECTS) --static -lz -o minisat_cubing

$(BUILDDIRS):
	mkdir -p $(BUILDDIRS)

build/%.o: %.cc $(BUILDDIRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@
