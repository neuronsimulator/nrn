NEURON {
    SUFFIX side_effects
    RANGE x, forward_flux, backward_flux
    NONSPECIFIC_CURRENT il
}

ASSIGNED {
    il
    x
    forward_flux
    backward_flux
}

STATE {
    X
    Y
}

INITIAL {
    X = 1
    Y = 2
}

BREAKPOINT {
    SOLVE state METHOD sparse

    il = forward_flux - backward_flux
}

KINETIC state {
    ~ X <-> Y (0.4, 0.5)
    forward_flux = f_flux
    backward_flux = b_flux

    x = X
}
