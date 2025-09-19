NEURON {
    SUFFIX cnexp_to_derivimplicit
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
    x' = -x/(1 + x)
}
