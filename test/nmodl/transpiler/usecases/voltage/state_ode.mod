NEURON {
    SUFFIX state_ode
    NONSPECIFIC_CURRENT il
}

STATE {
    X
}

ASSIGNED {
    il
    v
}

INITIAL {
    X = v
}

BREAKPOINT {
    SOLVE eqn
    il = 0.001 * X
}

NONLINEAR eqn { LOCAL c
    c = rate()
    ~ X = c
}

FUNCTION rate() {
    rate = v
}
