NEURON {
    SUFFIX rantst
    RANGE x
    RANDOM123 rrr
}

ASSIGNED {
    x
}

INITIAL {
    nrnran123_setseq(rrr, 5, 0)
    x = nrnran123_uniform(rrr, 0, 1)
}

BEFORE STEP {
    x = nrnran123_normal(rrr)
}


