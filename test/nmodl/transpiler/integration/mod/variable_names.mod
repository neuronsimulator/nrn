: Collection of tricky variable names.

NEURON {
    SUFFIX variable_names
}

STATE {
    is
    be
    count
    as
    del
    elif
    in
    pass

    alpha
    beta
    gamma
    delta
    epsilon
    zeta
    eta
    theta
    iota
    kappa
    lambda
    mu
    nu
    xi
    omicron
    : pi
    chi
    psi
    omega
}

BREAKPOINT {
    SOLVE state METHOD sparse
}

KINETIC state {
    ~ is <-> be (1, 1)
    ~ count <-> as (1, 1)
    ~ elif <-> in (1, 1)
    ~ lambda <-> pass (1, 1)
    ~ alpha <-> beta (1, 1)
    ~ gamma <-> delta (1, 1)
    ~ epsilon <-> zeta (1, 1)
    ~ eta <-> theta (1, 1)
    ~ iota <-> kappa (1, 1)
    ~ lambda <-> mu (1, 1)
    ~ nu <-> xi (1, 1)
    ~ omicron <-> chi (1, 1)
    ~ psi <-> omega (1, 1)
}
