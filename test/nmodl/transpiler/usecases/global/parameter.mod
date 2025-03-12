NEURON {
    SUFFIX parameter
}

PARAMETER {
    gbl = 42.0
}

FUNCTION get_gbl() {
    get_gbl = gbl
}
