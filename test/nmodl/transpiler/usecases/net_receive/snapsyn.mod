NEURON {
    POINT_PROCESS SnapSyn
    RANGE e, i
    NONSPECIFIC_CURRENT i
}

UNITS {
    (nA) = (nanoamp)
    (mV) = (millivolt)
    (uS) = (microsiemens)
}

PARAMETER {
    e = 10	(mV)
}

ASSIGNED {
    v (mV)
    i (nA)
    g (uS)
}

INITIAL {
    g=0
}

BREAKPOINT {
    i = g*(v - e)
}

NET_RECEIVE(weight (uS)) {
    g = g + weight
}
