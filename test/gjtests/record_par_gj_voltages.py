#!/usr/bin/env python3
"""Record test_par_gj dend voltages to a .npy file. Args: native(0|1) output.npy"""
import sys

import numpy as np
from neuron import h, gpu

import test_par_gj as t

native = bool(int(sys.argv[1]))
out_path = sys.argv[2]

pc = h.ParallelContext()
t.mkcells(pc, 4)
t.mkgjs(pc, 4)
pc.setup_transfer()
gpu.enable = native
if native:
    gpu.backend = "native"
h.dt = 0.25
pc.set_maxstep(10)
h.finitialize(-65)
pc.psolve(500)
np.save(out_path, np.column_stack([v.to_python() for v in t.vrecs]))
