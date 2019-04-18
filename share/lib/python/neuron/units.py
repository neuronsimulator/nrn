# -*- coding: utf-8 -*-
''' unit definitions '''

# NEURON's default units
mM = 1
ms = 1
mV = 1

# concentration
uM = 1e-3 * mM
nM = 1e-6 * mM
M = 1e3 * mM

# time
us = 1e-3 * ms
sec = 1e3 * ms
minute = 60 * sec
hour = 60 * minute
day = 24 * hour

# voltage
uV = 1e-3 * mV
V = 1e3 * mV

# variants using mu instead of u (available in Python3 only)
# have to do it this way to avoid a SyntaxError in Python2
globals()['μV'] = 1e-3 * mV
globals()['μM'] = 1e-3 * mM
globals()['μs'] = 1e-3 * ms
