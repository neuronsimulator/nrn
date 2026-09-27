NEURON {
    SUFFIX heat_eqn_function
    RANGE x
    GLOBAL g_mu, g_vol
    THREADSAFE
}

ASSIGNED {
    x
    g_mu
    g_vol
}

STATE {
    X
}

INITIAL {
    g_mu = 1.1
    g_vol = 0.01

    if(x < 0.5) {
        X = 1.0
    } else {
        X = 0.0
    }
}

BREAKPOINT {
    SOLVE state METHOD sparse
}

FUNCTION factor(x) {
    if(x < 0.25) {
        factor = 0.0
    } else {
        factor = 10*(x - 0.25)
    }
}

FUNCTION vol(x) {
    vol = (1 + x) * g_vol
}

FUNCTION mu(x) {
    mu = x * g_mu
}

KINETIC state {
    COMPARTMENT vol(x) {X}
    LONGITUDINAL_DIFFUSION mu(factor(x)) {X}

    : There must be a reaction equation, but
    : we only want to test diffusion.
    ~ X << (0.0)
}
