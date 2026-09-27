NEURON {
    SUFFIX global
    GLOBAL gbl
}

ASSIGNED {
    gbl
}

FUNCTION get_gbl() {
    get_gbl = gbl
}

PROCEDURE set_gbl(value) {
    gbl = value
}
