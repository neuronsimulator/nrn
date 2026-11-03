NEURON {
    SUFFIX accessors
    NONSPECIFIC_CURRENT il
}

ASSIGNED {
    v
    il
}

BREAKPOINT {
    il = 0.003
}


FUNCTION get_voltage() {
    get_voltage = v
}
