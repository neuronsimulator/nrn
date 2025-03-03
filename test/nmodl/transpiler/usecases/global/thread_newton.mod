NEURON {
    SUFFIX thread_newton
    RANGE x
    GLOBAL c
    THREADSAFE
}

ASSIGNED {
    c
    x
}

STATE {
    X
}

INITIAL {LOCAL total
    X = 0.0
    c = 42.0
}

BREAKPOINT {
    SOLVE state METHOD sparse
    x = X
}

KINETIC state {
    ~ X << (c)
}
