NEURON {
    SUFFIX rantst
    RANGE x
    RANDOM rrr
}

ASSIGNED {
    x
}

INITIAL {
    random_setseq(rrr, 5)
    x = random_uniform(rrr)
}

BEFORE STEP {
    x = random_normal(rrr)
}


