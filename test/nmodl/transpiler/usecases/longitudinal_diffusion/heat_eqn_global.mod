NEURON {
    SUFFIX heat_eqn_global
    RANGE x
}

PARAMETER {
    mu = 2.0
    vol = 0.01
}

ASSIGNED {
    x
}

STATE {
    X
}

INITIAL {
    if(x < 0.5) {
        X = 1.0
    } else {
        X = 0.0
    }
}

BREAKPOINT {
    SOLVE state METHOD sparse
}

KINETIC state {
    COMPARTMENT vol {X}
    LONGITUDINAL_DIFFUSION mu {X}

    : There must be a reaction equation, but
    : we only want to test diffusion.
    ~ X << (0.0)
}
