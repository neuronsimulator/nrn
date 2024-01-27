"""
Batch implementation suitable for parallel or serial
for stimulus current i iterating over a range of values
run a simulation and report spike frequency
save i & corresponding f to a file
optionally plot fi curve
"""

from mpi4py import MPI
from neuron import h
import time

# need the standard run system (didn't have to do this before, because this
# is loaded whenever we import the gui)
h.load_file("stdrun.hoc")

# create ParallelContext instance here so it exists whenever we need it
pc = h.ParallelContext()

# Simulation parameters

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
    """takes run i_d, returns corresponding stimulus amplitude"""
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


def fi(run_id):
    """set params, execute a simulation, analyze and return results"""
    # NB: run_id is sent via NEURON, which turns it into a float
    set_params(run_id)
    h.run()
    return (int(run_id), stim.amp, get_frequency(spvec))


# batch control

t_start = time.time()  # keep track of execution time past this point

# start execute loop on the workers
pc.runworker()

# code beyond this point is executed only by the master
# the master must now post jobs to the bulletin board


def batchrun(n):
    # preallocate lists of length n for storing parameters and results
    stims = [None] * n
    freqs = [None] * n
    for run_id in range(n):
        pc.submit(fi, run_id)
    count = 0
    while pc.working():
        run_id, amp, freq = pc.pyret()
        stims[run_id] = amp
        freqs[run_id] = freq
        count += 1
        print("Finished %d of %d." % (count, n))
    return stims, freqs


stims, freqs = batchrun(NRUNS)

pc.done()

# all simulations have been completed
# and the workers have been released
# but the boss still has things to do

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
