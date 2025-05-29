NEURON {
    SUFFIX X2Y
    RANGE X, Y, c1, c2
    NONSPECIFIC_CURRENT il
}

STATE {
    X
    Y
}

ASSIGNED {
    il
    i
    c1
    c2
}

BREAKPOINT {
    SOLVE state METHOD sparse
    il = i
}

INITIAL {
    X = 0
    Y = 1
    c1 = 0.0
    c2 = 0.0
}

KINETIC state {
    rates()
    ~ X <-> Y (c1, c2)
    i = (f_flux-b_flux)
}

PROCEDURE rates() {
    c1 = 0.4
    c2 = 0.5
}
