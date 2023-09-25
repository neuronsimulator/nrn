: for rangevar array test
NEURON {
    SUFFIX atst
    RANGE arrayrng, scalarrng
    GLOBAL arrayglob, scalarglob
}

DEFINE N 4

ASSIGNED {
    arrayrng[N]
    scalarrng
    arrayglob[N]
    scalarglob
}

FUNCTION farg0() {
    farg0 = 2
}

FUNCTION farg1(a1) {
    farg1 = 2*a1
}

FUNCTION farg2(a1, a2) {
    farg2 = a1 + a2
}
