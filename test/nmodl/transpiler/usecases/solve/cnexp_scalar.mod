NEURON {
    SUFFIX cnexp_scalar
}

STATE {
    x
}

INITIAL {
    x = 42
}

BREAKPOINT {
    SOLVE dX METHOD cnexp
}

DERIVATIVE dX {
    x' = -x
}

