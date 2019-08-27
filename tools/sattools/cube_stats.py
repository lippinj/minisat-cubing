from collections import defaultdict
import numpy as np


def read_cube_stats(path):
    """Read cube statistics from the given path.

    Returns a dict like:
    width -> yield -> count
    """
    ret = defaultdict(dict)
    with open(path) as f:
        for line in f:
            if line.startswith('INDET'):
                continue
            w, y, n = [int(e) for e in line.split()]
            ret[w][y] = n
    return dict(ret)


def calculate_scores(stats, scoring='p'):
    """Calculates the score distribution from cube statistics.

    Scorings for a cube of width w with yield y:
    'y' -- y
    'd' -- y / w
    'D' -- y / (w + 1)
    'p' -- (y + w) / w
    'P' -- (y + w) / (w + 1)

    Returns:
    S -- np.array of scores
    N -- np.array of occurrence counts
    C -- number of conflicted cubes
    """
    tmp = defaultdict(lambda: 0)
    C = 0

    if scoring == 'y':
        F = lambda w, y: y
    elif scoring == 'd':
        F = lambda w, y: y / w
    elif scoring == 'D':
        F = lambda w, y: y / (w + 1)
    elif scoring == 'p':
        F = lambda w, y: (y + w) / w
    elif scoring == 'P':
        F = lambda w, y: (y + w) / (w + 1)
    else:
        raise Exception('Unrecognized scoring method "{}"'.format(scoring))
    
    for w, yn in stats.items():
        for y, n in yn.items():
            if y > 0:
                s = F(w, y)
                tmp[s] += n
            else:
                assert(y == -1)
                C += n

    S = np.array(list(sorted(tmp.keys())))
    N = np.array([tmp[r] for r in S])
    m = np.dot(S, N) / np.sum(N)
    
    return S, N, C


def plot_score_distribution(ax, S, N, C, name='no name given',
                            dot_override={},
                            line_override={},
                            normalize=True,
                            show_title=True,
                            xlabel='score',
                            ylabel='count'):
    """Plot the distribution of scores.

    arguments:
    ax -- matplotlib axis
    S, N, C -- cube scores, as returned by calculate_scores()
    name -- identifying string for the instance
    dot_override -- override style for markers
    line_override -- override style for lines
    normalize -- normalize x axis?
    show_title -- generate a title?
    xlabel -- text on the x axis
    ylabel -- text on the y axis
    """
    dot_style = dict(marker='.', markersize=2, linestyle='')
    dot_style.update(dot_override)
    
    line_style = dict(linewidth=1.0, colors=(0.7, 0.7, 0.7))
    line_style.update(line_override)
    
    # Normalize data (if requested).
    mean = np.dot(S, N) / np.sum(N)
    if normalize:
        meanline = 1
        T = S / mean
    else:
        meanline = mean
        T = S
    
    # Draw:
    #  - red line to mark the mean score
    #  - vertical lines up to the occurrence counts
    #  - dots at the actual occurrence counts
    ax.axvline(meanline, color='r', linewidth=1.0, alpha=0.5)
    ax.vlines(T, 0, N, **line_style)
    ax.plot(T, N, **dot_style)
    ax.set_yscale('log')
    
    # Label axes and the graph, according to arguments
    if xlabel is not None:
        if normalize:
            xlabel += ' (normalized)'
        ax.set_xlabel(xlabel)
        
    if ylabel is not None:
        ax.set_ylabel(ylabel)
    
    if show_title:
        pct_c = 100.0 * float(C) / np.sum(N)
        ax.set_title('{}\n$\mu$={:.2f}; c={:.2f}%'.format(name, mean, pct_c))

