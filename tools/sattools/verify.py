import json

from . import benchmark
from . import cnf


def read_head(root):
    """The header is a JSON like:
      {
         timeout: 1000,
         runs: [
            ...
         ]
      }
    Where each run is like:
      {
          command_line: [strings...],
          elapsed_time: float,
          return_code: int,
          result: str
      }
    """
    path = root + '.json'
    with open(path) as f:
        return json.load(f)
    

def benchmarkify(j):
    """Makes into dicts like:
      {
          'benchmark_name': info,
          ...
      }
    Where the info is the same dict as the run above.
    """
    ret = {}
    for run in j['runs']:
        bm_path = run['command_line'][1].split('/')[-1]
        assert(bm_path not in ret)
        ret[bm_path] = run
    return ret


def refine(ref_roots, mod_roots):
    """Returns: ref_dicts, mod_dicts, benchmark_names"""
    ref_heads = [read_head(root) for root in ref_roots]
    mod_heads = [read_head(root) for root in mod_roots]
    refs = [benchmarkify(j) for j in ref_heads]
    mods = [benchmarkify(j) for j in mod_heads]

    names = None
    for bm in refs:
        names_here = list(sorted(bm.keys()))
        if names is None:
            names = names_here
        else:
            assert(names == names_here)

    return refs, mods, names


def check_model(instance_path, result_path):
    instance = cnf.Cnf.from_file(instance_path)
    assignment = benchmark.read_minisat_result(result_path)
    return instance.evaluate(assignment)


def verify_one(refs, mods, name, ipath_fn=None, rpath_fn=None):
    ref_rcs = [B[name]['return_code'] for B in refs]
    mod_rcs = [B[name]['return_code'] for B in mods]

    # Find the return code from the reference solver. It should be the same
    # across all runs that did not end in a timeout.
    ref_res = ''
    ref_rc = None
    for i, rc in enumerate(ref_rcs):
        if rc == 10:
            ref_res += 'S'
            if ref_rc is None:
                ref_rc = 10
            else:
                assert(rc == ref_rc)
        elif rc == 20:
            ref_res += 'U'
            if ref_rc is None:
                ref_rc = 20
            else:
                assert(rc == ref_rc)
        else:
            ref_res += '-'
            assert(refs[i][name]['result'] == 'TIMEOUT')

    # Same for the modified solver.
    bad_models = []
    bad_codes = []
    mismatches = False
    mod_res = ''
    mod_rc = None
    for i, rc in enumerate(mod_rcs):
        if rc == 10:
            mod_res += 'S'
            if ipath_fn is not None:
                instance_path = ipath_fn(name)
                result_path = rpath_fn(name, i)
                if not check_model(instance_path, result_path):
                    bad_models.append(i)
            if mod_rc is None:
                mod_rc = 10
            else:
                if rc != mod_rc:
                    mismatches = True

                assert(rc == mod_rc)
        elif rc == 20:
            mod_res += 'U'
            if mod_rc is None:
                mod_rc = 20
            else:
                if rc != mod_rc:
                    mismatches = True
        elif mods[i][name]['result'] == 'TIMEOUT':
            mod_res += '-'
        else:
            bad_codes.append((i, rc))
            mod_res += 'E'

    agreement = True
    if mod_rc is not None and ref_rc is not None:
        agreement = (mod_rc == ref_rc)

    return ref_res, mod_res, agreement, mismatches, bad_models, bad_codes


def print_verify_spacer(N):
    N = max(N, 8)
    print('|=====|={:<32}=|={}=|={}=|========+'.format('=' * 32, '=' * N, '=' * N))

def print_verify_head(N):
    N = max(N, 8)
    print_verify_spacer(N)
    print('|  #  | {:<32} | {} | {} | result |'.format(
        '  Instance', 'ref.'.center(N), 'mod.'.center(N)))
    print_verify_spacer(N)

def print_verify_row(n, name, ref, mod, agree, mis, bad_models, bad_codes):
    meta = []
    if not agree:
        meta.append('disagree')
    if mis:
        meta.append('inconsistent')
    for i in bad_models:
        meta.append('bad-model-{}'.format(i))
    for i, rc in bad_codes:
        meta.append('bad-code-{}={}'.format(i, rc))
    if len(meta) == 0:
        meta = 'ok'
    else:
        meta = ' '.join(meta)
    print('|{:>4} | {:<32} | {} | {} | {}'.format(n, name, ref.rjust(8), mod.rjust(8), meta))

