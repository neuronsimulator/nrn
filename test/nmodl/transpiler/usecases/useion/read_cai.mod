NEURON {
    SUFFIX read_cai
    USEION ca READ cai
    RANGE x
}

ASSIGNED {
    x
    cai
}

INITIAL {
    x = cai
}
