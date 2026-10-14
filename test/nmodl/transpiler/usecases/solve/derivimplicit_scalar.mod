NEURON {
    SUFFIX derivimplicit_scalar
}

STATE {
    x
}

INITIAL {
    x = 42
}

BREAKPOINT {
    SOLVE dX METHOD derivimplicit
}

DERIVATIVE dX {
    x' = -x
}

