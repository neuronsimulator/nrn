NEURON {
    SUFFIX heat_eqn_thread_vars
    RANGE x
    GLOBAL mu, vol
    THREADSAFE
}

ASSIGNED {
    x
    mu
    vol
}

STATE {
    X
}

INITIAL {
    mu = 1.1
    vol = 0.01

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
