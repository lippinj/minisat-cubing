import os
import random
import time

from . import timed_run


def instances_in(directory):
    """Returns (full) paths to all instances in the given directory
    (really, all files ending in .cnf or .cnf.gz).
    """
    items = os.listdir(directory)
    instances = [e for e in items if e.endswith('.cnf') or e.endswith('.cnf.gz')]
    instances = ['{}/{}'.format(directory, e) for e in instances]
    return instances


def benchmark_batch(solver_path, instance_paths, results_dir, timeout=1000, threads=2, seed=None, solver_args=[]):
    """Benchmark a batch of instances by TimedRunBatchPW. Command lines are like:
        [solver] [instance] [resultdir]/[instancename].result -rnd-seed=[seed]

    The commands are run in shuffled order. Results are saved to [results_dir].json.
    """
    cmds = []
    for path in instance_paths:
        name = os.path.basename(path)
        result_path = '{}/{}.result'.format(results_dir, name)
        stdout_path = '{}/{}.stdout'.format(results_dir, name)
        cmd = [solver_path, path, result_path] + solver_args
        if seed is not None:
            cmd.append('-rnd-seed={}'.format(seed))
        cmds.append((cmd, stdout_path))
    random.shuffle(cmds)

    stdouts = [o for c,o in cmds]
    cmds = [c for c,o in cmds]
    
    start_time = time.time()
    os.mkdir(results_dir)
    runs = timed_run.TimedRunBatchPW(cmds, 'partial.json', timeout, threads, outfiles=stdouts)
    runs.run()
    runs.to_json('{}.json'.format(results_dir))
    end_time = time.time()
    
    total_seconds = end_time - start_time
    total_hours = total_seconds / 3600.0
    print('Total time: {:.3f}h'.format(total_hours))


def read_minisat_result(result_path):
    """Read the output from minisat.

    Returs one of:
    None -- empty file
    'UNSAT' -- unsatisfiable result
    dict containing the assignment -- satisfiable result
    """
    with open(result_path) as f:
        s = f.read().rstrip()
        head = s.split()[0]
        if len(s) == 0:
            return None
        if s.startswith('UNSAT'):
            return 'UNSAT'
        elif s.startswith('INDET'):
            return 'INDET'
        elif s.startswith('SAT'):
            A = {}
            s = [int(e) for e in s.split()[1:]]
            for e in s:
                if e < 0:
                    A[-e] = False
                else:
                    A[e] = True
            return A
        else:
            raise Exception('Unknown result {}'.format(head))

