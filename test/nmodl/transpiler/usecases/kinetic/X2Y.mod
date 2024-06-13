NEURON {
    SUFFIX X2Y
    RANGE X, Y
    NONSPECIFIC_CURRENT il
}

STATE {
    X
    Y
}

ASSIGNED {
    il
    i
}

BREAKPOINT {
    SOLVE state METHOD sparse
    il = i
}

INITIAL {
    X = 0
    Y = 1
}

KINETIC state {
    ~ X <-> Y (0.4, 0.5)
    i = (f_flux-b_flux)
}
