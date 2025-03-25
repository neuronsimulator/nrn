NEURON {
    SUFFIX read_only
    GLOBAL c
}

PARAMETER {
    c = 2.0
}

STATE {
    x
}

INITIAL {
    x = 42
}

BREAKPOINT {
    x = c
}
