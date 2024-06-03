NEURON {
    SUFFIX tbl
    NONSPECIFIC_CURRENT i
    RANGE e, g, gmax, v1, v2
    GLOBAL k, d, c1, c2
}

PARAMETER {
    e = 0
    gmax = 0
    k = .1
    d = -50
    c1 = 1
    c2 = 2
}

ASSIGNED {
    g
    i
    v
    sig
    v1
    v2
}

BREAKPOINT {
    sigmoid1(v)
    g = gmax * sig
    i = g*(v - e)
}

PROCEDURE sigmoid1(v) {
    TABLE sig DEPEND k, d FROM -127 TO 128 WITH 155
    sig = 1/(1 + exp(k*(v - d)))
}

FUNCTION quadratic(arg) {
    TABLE DEPEND c1, c2 FROM -3 TO 5 WITH 500
    quadratic = c1 * arg * arg + c2
}

PROCEDURE sinusoidal(arg) {
    TABLE v1, v2 DEPEND c1, c2 FROM -4 TO 6 WITH 300
    v1 = sin(c1 * arg) + 2
    v2 = cos(c2 * arg) + 2
}
