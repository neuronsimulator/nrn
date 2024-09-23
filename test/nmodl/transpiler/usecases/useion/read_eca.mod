NEURON {
    SUFFIX read_eca
    USEION ca READ eca
    RANGE x
}

ASSIGNED {
    x
    eca
}

INITIAL {
    x = eca
}
