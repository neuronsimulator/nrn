"""
Serial batch implementation
for stimulus current i iterating over a range of values
run a simulation and report spike frequency
save i & corresponding f to a file
optionally plot fi curve
"""

from neuron import h, gui
import time

# Simulation parameters

PLOTRESULTS = True  # if True, generates a graph that shows f-i curve
h.tstop = 500  # ms, more than long enough for 15 spikes at ISI = 25 ms
AMP0 = 0.1  # nA -- minimum stimulus
D_AMP = 0.02  # nA -- stim increment between runs
NRUNS = 30

# model specification
from cell import Cell

cell = Cell()

# instrumentation

# experimental manipulations
stim = h.IClamp(cell.soma(0.5))
stim.delay = 1  # ms
stim.dur = 1e9
stim.amp = 0.1  # nA


def fstimamp(run_id):
    """takes run_id, returns corresponding stimulus amplitude"""
    return AMP0 + run_id * D_AMP


def set_params(run_id):
    """sets stimulus amplitude to appropriate value for this run"""
    stim.amp = fstimamp(run_id)


# data recording and analysis

# count only those spikes that get to distal end of dend
nc = h.NetCon(cell.dend(1)._ref_v, None, sec=cell.dend)
nc.threshold = -10  # mV
spvec = h.Vector()
nc.record(spvec)

NSETTLE = 5  # ignore the first NSETTLE ISI (allow freq to stablize)
NINVL = 10  # num ISI from which frequency will be calculated
NMIN = NSETTLE + NINVL  # ignore recordings with fewer than this num of ISIs


def get_frequency(spvec):
    nspikes = spvec.size()
    if nspikes > NMIN:
        t2 = spvec[-1]  # last spike
        t1 = spvec[-(1 + NINVL)]  # NINVL prior to last spike
        return NINVL * 1.0e3 / (t2 - t1)
    else:
        return 0


#
# Simulation control
#

# batch control
t_start = time.time()  # keep track of execution time past this point


def batchrun(n):
    stims = []
    freqs = []
    for run_id in range(n):
        set_params(run_id)
        h.run()
        stims.append(stim.amp)
        freqs.append(get_frequency(spvec))
        print("Finished %d of %d." % (run_id + 1, n))
    return stims, freqs


stims, freqs = batchrun(NRUNS)

#
# Reporting of results
#


def save_results(filename, stims, freqs):
    with open(filename, "w") as f:
        f.write("label:%s\n%d\n" % ("f", len(freqs)))
        for stim, freq in zip(stims, freqs):
            f.write("%g\t%g\n" % (stim, freq))


save_results("fi.dat", stims, freqs)

print("%g seconds" % (time.time() - t_start))  # report run time

if PLOTRESULTS:
    from plotfi import plotfi

    fig = plotfi(stims, freqs)
