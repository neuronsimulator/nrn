NEURON {
    SUFFIX pointing
    POINTER ptr
}

ASSIGNED {
    ptr
}

FUNCTION is_valid() {
    if (nrn_pointing(ptr)) {
        is_valid = 1.0
    } else {
        is_valid = 0.0
    }
}
