NEURON {
    SUFFIX top_local
    NONSPECIFIC_CURRENT il
    RANGE y
}

ASSIGNED {
    y
    il
}

LOCAL gbl

INITIAL {
    gbl = 2.0
}

BREAKPOINT {
    if(t > 0.33) {
        gbl = 3.0
    }

    y = gbl
    il = 0.0000001 * (v - 10.0)
}
