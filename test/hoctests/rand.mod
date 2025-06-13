NEURON {
    SUFFIX rantst
    RANGE x1, x2
    RANDOM ran1, ran2
}

ASSIGNED {
    x1 x2
}

INITIAL {
    random_setseq(ran1, 5)
    random_setseq(ran2, 5)
    x1 = random_uniform(ran1)
}

BEFORE STEP {
    x2 = random_normal(ran2)
}

FUNCTION uniform0() {
    uniform0 = random_uniform(ran1)
}

FUNCTION uniform2(min, max) {
    uniform2 = random_uniform(ran1, min, max)
}

FUNCTION negexp0() {
    negexp0 = random_negexp(ran1)
}

FUNCTION negexp1(mean) {
    negexp1 = random_negexp(ran1, mean)
}

FUNCTION normal0() {
    normal0 = random_normal(ran1)
}

FUNCTION normal2(mean, std) {
    normal2 = random_normal(ran1, mean, std)
}
