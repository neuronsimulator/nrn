NEURON {
    SUFFIX ode
    NONSPECIFIC_CURRENT il
}

ASSIGNED {
    il
    v
}

FUNCTION voltage() {
    voltage = 0.001 * v
}

BREAKPOINT {
    il = voltage()
}
