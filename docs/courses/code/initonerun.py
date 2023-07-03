"""
Executes one simulation with specified stimulus.
Displays response and reports spike frequency
"""

from neuron import h, gui

# simulation parameter
h.tstop = 500  # ms, more than long enough for 15 spikes at ISI = 25 ms

# model specification
from cell import Cell

cell = Cell()

# instrumentation

# experimental manipulations
stim = h.IClamp(cell.soma(0.5))
stim.delay = 1  # ms
stim.dur = 1e9
stim.amp = 0.1  # nA

# data recording and analysis
# count only those spikes that get to distal end of dend
nc = h.NetCon(cell.dend(1)._ref_v, None, sec=cell.dend)
nc.threshold = -10  # mV
spvec = h.Vector()
nc.record(spvec)

NSETTLE = 5  # ignore the first NSETTLE ISI (allow freq to stablize)
NINVL = 10  # num ISI from which frequency will be calculated
NMIN = NSETTLE + NINVL  # ignore recordings with fewer than this num of ISIs


def get_frequency():
    nspikes = spvec.size()
    if nspikes > NMIN:
        t2 = spvec[-1]  # last spike
        t1 = spvec[-(1 + NINVL)]  # NINVL prior to last spike
        return NINVL * 1.0e3 / (t2 - t1)
    else:
        return 0


# Simulation control and reporting of results


def onerun(amp):
    # amp = amplitude of stimulus current
    g = h.Graph(0)
    g.size(0, 500, -80, 40)
    g.view(0, -80, 500, 120, 2, 105, 300.48, 200.32)
    # update graph throughout the simulation
    h.graphList[0].append(g)
    # plot v at distal end of dend
    g.addvar("dend(1).v", cell.dend(1)._ref_v)
    stim.amp = amp
    h.run()
    freq = get_frequency()
    print("stimulus %g frequency %g" % (amp, freq))


onerun(0.15)
