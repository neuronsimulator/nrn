NEURON {
    SUFFIX write_cai
    USEION ca WRITE cai
}

ASSIGNED {
    cai
}

INITIAL {
    cai = 1124.0
}
