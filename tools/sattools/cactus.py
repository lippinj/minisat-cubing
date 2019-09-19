import json
import math
import os
import numpy as np
from matplotlib import pyplot as plt
from matplotlib.ticker import MaxNLocator


def read_summary(filename):
    """Reads the summary dict from a JSON file."""
    with open(filename) as f:
        return json.load(f)


def simple_cactus_of(summary):
    """Returns a cactus from a single summary (no averaging etc.,
    just believe the values for any non-timeout result with a valid
    retcode).
    """
    tt = []
    for run in summary['runs']:
        if run['result'] != 'OK': continue
        if run['return_code'] not in (10, 20): continue
        tt.append(run['elapsed_time'])
    return sorted(tt)


def cactus_plot(ax, cacti, grid=False):
    """Plot a cactus plot from the given data.
    
    Args:
    ax -- the pyplot axis to draw to
    cacti -- a list of (label, data) items
    """
    for tt, kwargs in cacti:
        if 'linestyle' not in kwargs: kwargs['linestyle'] = ''
        if 'marker' not in kwargs: kwargs['marker'] = 'x'
        xx = [(i + 1) for i in range(len(tt))]
        ax.plot(xx, tt, **kwargs)
    ax.legend()
        
    ax.grid(grid)
    ax.set_ylim([0, None])
    ax.set_xlim([0, None])
    ax.xaxis.set_major_locator(MaxNLocator(integer=True))
    
    ax.set_xlabel('Instances solved')
    ax.set_ylabel('Wall-clock time (s)')


def instance_name_from(run):
    """Extract the instance name from the command line array of a run."""
    cmd = run['command_line']
    cmd = [e for e in cmd[1:] if not e.startswith('-')]
    return os.path.basename(cmd[0])


def aggregate_summary_row_of(entry, key, n):
    """Helper function."""
    if key not in entry:
        row = [None] * n
        entry[key] = row
    else:
        row = entry[key]
        row += [None] * (n - len(row))
        entry[key] = row
    return row


def aggregate_summaries(summaries):
    """Construct an aggregate of the given N summaries.

    Returned value is a dict of items like:
        instancename ->
            {
                'time': [time1, time2, ..., timeN],
                'code': [retcode1, retcode2, ..., retcodeN]
            }

    Missing values are None.
    """
    ret = {}
    for i, summary in enumerate(summaries):
        for run in summary['runs']:
            name = instance_name_from(run)
            if name not in ret:
                ret[name] = {}
            entry = ret[name]
            
            row = aggregate_summary_row_of(entry, 'time', i)
            time = run['elapsed_time']
            if run['result'] == 'OK':
                row.append(time)
            else:
                row.append(None)
                
            row = aggregate_summary_row_of(entry, 'code', i)
            code = run['return_code']
            if run['result'] == 'OK':
                row.append(code)
            else:
                row.append(None)

    for name in ret:
        aggregate_summary_row_of(ret[name], 'time', len(summaries))
        aggregate_summary_row_of(ret[name], 'code', len(summaries))
            
    return ret


def relative_comparison(A):
    """Make a relative comparison between an aggregate with N=2. The
    solver at index 0 is "X" and the one at index 1 is "Y".
    
    Output is a dict with the keys:
    x_only -- list of (name, code) tuples of instances only solved by X
    x_only -- list of (name, code) tuples of instances only solved by Y
    unsolveds -- list of names of instances only solved by Y
    mismatches -- list of (name, xcode, ycode) tuples of instances
                  where the solvers disagree on the return code
    x_bad_codes -- list of (name, code) tuples of instances where X had
                   a non-standard return code
    y_bad_codes -- list of (name, code) tuples of instances where Y had
                   a non-standard return code
    x_unsat -- list of X's solving times for UNSAT instances
    y_unsat -- list of Y's solving times for UNSAT instances
    x_sat -- list of X's solving times for SAT instances
    y_sat -- list of Y's solving times for SAT instances
    """
    # Make sure that the dimensions in the data are correct
    for name, entry in A.items():
        assert(len(entry['code']) == 2)
        assert(len(entry['time']) == 2)
        
    # Organize data
    x_only = []
    y_only = []
    unsolveds = []
    mismatches = []
    x_bad_codes = []
    y_bad_codes = []
    
    x_unsat = []
    y_unsat = []
    x_sat = []
    y_sat = []
    
    for name, entry in A.items():
        codes = entry['code']
        times = entry['time']

        if codes[0] not in (10, 20, None):
            x_bad_codes.append((name, codes[0]))
            continue
        if codes[1] not in (10, 20, None):
            y_bad_codes.append((name, codes[1]))
            continue
        if codes[0] is None and codes[1] is None:
            unsolveds.append(name)
            continue
        if codes[0] is None:
            x_only.append((name, codes[0]))
            continue
        if codes[1] is None:
            y_only.append((name, codes[1]))
            continue
        if codes[0] != codes[1]:
            mismatches.append((name, codes[0], codes[1]))
            continue

        code = codes[0]
        x_time = times[0]
        y_time = times[1]

        if code == 10:
            x_sat.append(x_time)
            y_sat.append(y_time)
        else:
            x_unsat.append(x_time)
            y_unsat.append(y_time)

    # Return in a dict
    ret = {}
    ret['x_only'] = x_only
    ret['y_only'] = y_only
    ret['unsolveds'] = unsolveds
    ret['mismatches'] = mismatches
    ret['x_bad_codes'] = x_bad_codes
    ret['y_bad_codes'] = y_bad_codes
    ret['x_unsat'] = x_unsat
    ret['y_unsat'] = y_unsat
    ret['x_sat'] = x_sat
    ret['y_sat'] = y_sat
    return ret

    
def relative_comparison_plot_diag(A, ax, timeout=None, xname='Solver X', yname='Solver Y', tau=0.8, fontsize=15):
    """Draws a comparison plot from the relative comparison A.

    A log-log plot of solver Y time as a function of solver X time is
    drawn. (Perfectly equal solvers draw a diagonal, y=x.)
    """
    x_sat = A['x_sat']
    y_sat = A['y_sat']
    x_unsat = A['x_unsat']
    y_unsat = A['y_unsat']

    # If no explicit timeout was given, use the longest time from the data
    if timeout is None:
        timeout_sat = max(max(x_sat), max(y_sat))
        timeout_unsat = max(max(x_unsat), max(y_unsat))
        timeout = max(timeout_sat, timeout_unsat)
    
    # Draw the guide lines:
    # Equal line
    # 2x line
    # 10x line
    lims = np.array([0.01, timeout])
    ax.plot(lims, lims, linestyle=':', linewidth=1.0, color='k', alpha=0.75)
    ax.plot(lims, 2.0 * lims, linestyle=':', linewidth=1.0, color='k', alpha=0.5)
    ax.plot(lims, 0.5 * lims, linestyle=':', linewidth=1.0, color='k', alpha=0.5)
    ax.plot(lims, 10.0 * lims, linestyle=':', linewidth=1.0, color='k', alpha=0.25)
    ax.plot(lims, 0.10 * lims, linestyle=':', linewidth=1.0, color='k', alpha=0.25)
    ax.set_xlim(lims)
    ax.set_ylim(lims)
    ax.set_yscale('log')
    ax.set_xscale('log')
    ax.grid(True)

    ax.plot(x_sat, y_sat, linestyle='', marker='o', fillstyle='none', label='SAT')
    ax.plot(x_unsat, y_unsat, linestyle='', marker='x', color='r', fillstyle='none', label='UNSAT')
    ax.legend(loc='lower center')

    ax.set_xlabel('{} time (s)'.format(xname))
    ax.set_ylabel('{} time (s)'.format(yname))

    a = math.log(lims[0], 10.0)
    b = math.log(lims[1], 10.0)
    A = math.pow(10.0,         (tau * a) + ((1.0 - tau) * b))
    B = math.pow(10.0, ((1.0 - tau) * a) +         (tau * b))

    props = dict(va='center', ha='center', rotation=45, size=fontsize, alpha=0.5)
    ax.text(B, A, '{} is faster'.format(yname), **props)
    ax.text(A, B, '{} is faster'.format(xname), **props)

        
def relative_comparison_plot_horizontal(A, ax, timeout=None, xname='Solver X', yname='Solver Y', tau=0.9, fontsize=20):
    """Draws a comparison plot from the relative comparison A.

    A log-log plot of solver Y relative time as a function of solver X
    time is drawn. (Perfectly equal solvers draw a horizontal line, y=1.)
    """
    x_sat = A['x_sat']
    y_sat = A['y_sat']
    x_unsat = A['x_unsat']
    y_unsat = A['y_unsat']

    # If no explicit timeout was given, use the longest time from the data
    if timeout is None:
        timeout_sat = max(max(x_sat), max(y_sat))
        timeout_unsat = max(max(x_unsat), max(y_unsat))
        timeout = max(timeout_sat, timeout_unsat)
    xlims = np.array([0.01, timeout])

    # Get relative solving times
    z_sat = [y / x for x, y in zip(x_sat, y_sat)]
    z_unsat = [y / x for x, y in zip(x_unsat, y_unsat)]

    zmax = max(max(z_sat), max(z_unsat))
    zmin = min(min(z_sat), min(z_unsat))
    zamax = max(zmax, 1.0 / zmin) * 5.0
    zlims = np.array([1.0 / zamax, zamax])
    #zlims = np.array([0.01, 100.0])

    ax.plot(xlims, [ 1.0,  1.0], linestyle=':', linewidth=1.0, color='k', alpha=0.75)
    ax.plot(xlims, [ 2.0,  2.0], linestyle=':', linewidth=1.0, color='k', alpha=0.50)
    ax.plot(xlims, [ 0.5,  0.5], linestyle=':', linewidth=1.0, color='k', alpha=0.50)
    ax.plot(xlims, [10.0, 10.0], linestyle=':', linewidth=1.0, color='k', alpha=0.25)
    ax.plot(xlims, [ 0.1,  0.1], linestyle=':', linewidth=1.0, color='k', alpha=0.25)
    
    ax.set_xlim(xlims)
    ax.set_ylim(zlims)
    ax.set_yscale('log')
    ax.set_xscale('log')
    ax.xaxis.grid(True)

    ax.plot(x_sat, z_sat, linestyle='', marker='o', fillstyle='none', label='SAT')
    ax.plot(x_unsat, z_unsat, linestyle='', marker='x', color='r', fillstyle='none', label='UNSAT')
    ax.legend()

    ax.set_xlabel('{} time (s)'.format(xname))
    ax.set_ylabel('{} time (relative)'.format(yname))
    # ax.yaxis.set_ticks((0.1, 0.5, 1.0, 2.0, 10.0), ('0.1x', '0.5x', '1x', '2x', '10x'))

    a = math.log(zlims[0], 10.0)
    b = math.log(zlims[1], 10.0)
    A = math.pow(10.0,         (tau * a) + ((1.0 - tau) * b))
    B = math.pow(10.0, ((1.0 - tau) * a) +         (tau * b))
    C = math.pow(10.0, 0.5 * (math.log(xlims[0], 10.0) + math.log(xlims[1], 10.0)))

    props = dict(va='center', ha='center', size=fontsize, alpha=0.5)
    ax.text(C, A, '{} is faster'.format(yname), **props)
    ax.text(C, B, '{} is faster'.format(xname), **props)
        
