NEURON {
    POINT_PROCESS ExpSyn
    RANGE tau, e, i
    NONSPECIFIC_CURRENT i
}

UNITS {
    (nA) = (nanoamp)
    (mV) = (millivolt)
    (uS) = (microsiemens)
}

PARAMETER {
    tau = 0.1 (ms) <1e-9,1e9>
    e = 0	(mV)
}

ASSIGNED {
    v (mV)
    i (nA)
}

STATE {
    g (uS)
}

INITIAL {
    g = 0
}

BREAKPOINT {
    SOLVE state METHOD cnexp
    i = g*(v - e)
}

DERIVATIVE state {
    g' = -g/tau
}

NET_RECEIVE(weight (uS)) {
    g = g + weight
}
