NEURON {
    SUFFIX write_eca
    USEION ca WRITE eca
}

ASSIGNED {
    eca
}

INITIAL {
    eca = 1124.0
}
