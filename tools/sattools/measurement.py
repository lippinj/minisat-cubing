import datetime
import os
import time
import shutil

from . import benchmark
from . import verify
from .cnf import Cnf


class Measurement:

    def __init__(self,
                 benchmarks_dir, 
                 tmp_benchmarks_dir, 
                 original_solver_path,
                 modified_solver_path,
                 results_dir):
        """Constructor."""
        self.benchmarks_dir = benchmarks_dir
        self.tmp_benchmarks_dir = tmp_benchmarks_dir
        self.original_solver_path = original_solver_path
        self.modified_solver_path = modified_solver_path
        self.results_dir = results_dir

        # Benchmarks
        assert(os.path.exists(benchmarks_dir))
        assert(os.path.isdir(benchmarks_dir))
        self.benchmark_names = []
        self.benchmark_filenames = []
        for f in os.listdir(benchmarks_dir):
            if f.endswith('.cnf.gz'):
                name = f[:-7]
                self.benchmark_names.append(name)
                self.benchmark_filenames.append(f)

        # Executables
        assert(os.path.exists(original_solver_path))
        assert(os.path.isfile(original_solver_path))
        assert(os.path.exists(modified_solver_path))
        assert(os.path.isfile(modified_solver_path))

        # Results
        if not os.path.exists(results_dir):
            os.mkdir(results_dir)

        self.threads = 2
        self.timeout = 1000
        self.original_args = []
        self.modified_args = ['-k_c=2', '-no-always-search', 'k_t=10']

        # Misc
        self.seed_randomize = None
        self.duration_randomize = None
        self.duration_solve_original = None
        self.duration_solve_modified = None
        self.duration_check = None
        self.duration_verify = None
        self.tags = []

    def print_hello(self):
        print('=== Measurement corpus ===')
        print('benchmark count: {}'.format(len(self.benchmark_names)))
        print('benchmark dir:   {}'.format(self.benchmarks_dir))
        print('bounce dir:      {}'.format(self.tmp_benchmarks_dir))
        print('original solver: {} {}'.format(
            self.original_solver_path, ' '.join(self.original_args)))
        print('modified solver: {} {}'.format(
            self.modified_solver_path, ' '.join(self.modified_args)))
        print('results dir:     {}'.format(self.results_dir))
        print('thread count:    {}'.format(self.threads))
        print('timeout:         {} s'.format(self.timeout))

    def print_randomized(self):
        s = str(datetime.timedelta(seconds=self.duration_randomize))
        if self.seed_randomize is None:
            print('Copied in {}.'.format(s))
        else:
            print('Randomized in {}.'.format(s))

    def run_one(self, solver_seed, randomize_seed, tag=None):
        if tag is None:
            tag = str(solver_seed)
        self.tags.append(tag)

        self.randomize(randomize_seed)
        self.print_randomized()

        self.solve_both(solver_seed, tag)
        self.check_both(tag)

        self.verify()

    def randomize(self, seed=None):
        t0 = time.monotonic()
        self.seed_randomize = seed

        # Remove existing temporary directory and recreate as empty
        shutil.rmtree(self.tmp_benchmarks_dir, ignore_errors=True)
        os.mkdir(self.tmp_benchmarks_dir)

        for name in self.benchmark_names:
            src = '{}/{}.cnf.gz'.format(self.benchmarks_dir, name)
            dst = '{}/{}.cnf.gz'.format(self.tmp_benchmarks_dir, name)

            # If no seed is given, copy everything over as-is
            if seed is None:
                shutil.copy(src, dst)

            # Otherwise, randomize
            else:
                src_cnf = Cnf.from_file(src)
                dst_cnf = src_cnf.shuffle(seed)
                dst_cnf.to_file(dst)

        t1 = time.monotonic()
        self.duration_randomize = t1 - t0

    def solve_both(self, solver_seed, tag):
        results_dir = '{}/modified_{}'.format(self.results_dir, tag)
        self.duration_solve_modified = self.solve_one(
                solver_seed,
                self.modified_solver_path,
                results_dir,
                self.modified_args,
                'Modified')

        results_dir = '{}/original_{}'.format(self.results_dir, tag)
        self.duration_solve_original = self.solve_one(
                solver_seed,
                self.original_solver_path,
                results_dir,
                self.original_args,
                'Original')

    def solve_one(self, solver_seed, solver_path, results_dir, args, desc):
        instance_paths = ['{}/{}.cnf.gz'.format(self.tmp_benchmarks_dir, name)\
                          for name in self.benchmark_names]

        t0 = time.monotonic()
        benchmark.benchmark_batch(
                solver_path,
                instance_paths,
                results_dir,
                self.timeout,
                self.threads,
                solver_seed,
                args)
        t1 = time.monotonic()
        s = str(datetime.timedelta(seconds=(t1 - t0)))
        print('{} solver time: {}'.format(desc, s))
        return t1 - t0

    def check_both(self, tag):
        t0 = time.monotonic()

        results_dir = '{}/original_{}'.format(self.results_dir, tag)
        self.check_one(results_dir, 'Original')

        results_dir = '{}/modified_{}'.format(self.results_dir, tag)
        self.check_one(results_dir, 'Modified')

        t1 = time.monotonic()
        self.duration_check = t1 - t0
        s = str(datetime.timedelta(seconds=(t1 - t0)))
        print('Time to check: {}'.format(s))

    def check_one(self, results_dir, desc):
        bads = []
        unsats = []
        indets = []
        sats = []
        for name in self.benchmark_names:
            bm = '{}/{}.cnf.gz'.format(self.tmp_benchmarks_dir, name)
            cnf = Cnf.from_file(bm)
            res = '{}/{}.cnf.gz.result'.format(results_dir, name)
            A = benchmark.read_minisat_result(res)
            if A is None:
                indets.append(name)
            elif A == 'UNSAT':
                unsats.append(name)
            elif A == 'INDET':
                indets.append(name)
            elif cnf.evaluate(A):
                sats.append(name)
            else:
                bads.append(name)

        ssat = '{} sat'.format(len(sats))
        sunsat = '{} unsat'.format(len(unsats))
        sindet = '{} indet'.format(len(indets))
        if len(bads) == 0:
            print('{} OK.'.format(desc))
            print(', '.join([ssat, sunsat, sindet]))
        else:
            print('{} had ERRORS.'.format(desc))
            for name in bads:
                print('Bad result: {}'.format(name))
            print('also: ' + ', '.join([ssat, sunsat, sindet]))

    def verify(self):
        t0 = time.monotonic()

        mod_roots = []
        ref_roots = []
        for tag in self.tags:
            mod_dir = '{}/modified_{}'.format(self.results_dir, tag)
            ref_dir = '{}/original_{}'.format(self.results_dir, tag)
            mod_roots.append(mod_dir)
            ref_roots.append(ref_dir)

        refs, mods, names = verify.refine(ref_roots, mod_roots)
        assert(set(names) == set(self.benchmark_filenames))

        i = 0
        name = ''
        for i, name in enumerate(names):
            if i % 20 == 0:
                verify.print_verify_head(len(self.tags))
            vee = verify.verify_one(refs, mods, name)
            verify.print_verify_row(i + 1, name, *vee)
        verify.print_verify_row(i + 1, name, *vee)
        verify.print_verify_spacer(len(self.tags))

        t1 = time.monotonic()
        self.duration_verify = t1 - t0

