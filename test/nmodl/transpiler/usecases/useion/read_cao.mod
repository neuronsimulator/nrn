NEURON {
    SUFFIX read_cao
    USEION ca READ cao
    RANGE x
}

ASSIGNED {
    x
    cao
}

INITIAL {
    x = cao
}
