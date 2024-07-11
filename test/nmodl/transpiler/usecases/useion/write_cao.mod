NEURON {
    SUFFIX write_cao
    USEION ca WRITE cao
}

ASSIGNED {
    cao
}

INITIAL {
    cao = 1124.0
}
