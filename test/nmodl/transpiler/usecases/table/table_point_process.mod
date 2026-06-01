NEURON {
    POINT_PROCESS tbl_point_process
    NONSPECIFIC_CURRENT i
    RANGE g, v1, v2
    GLOBAL k, d, c1, c2
}

PARAMETER {
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
    sigmoidal(v)
    g = 0.001 * sig
    i = g*(v - 30.0)
}

PROCEDURE sigmoidal(v) {
    TABLE sig DEPEND k, d FROM -127 TO 128 WITH 155
    sig = 1/(1 + exp(k*(v - d)))
}

FUNCTION quadratic(x) {
    TABLE DEPEND c1, c2 FROM -3 TO 5 WITH 500
    quadratic = c1 * x * x + c2
}

PROCEDURE sinusoidal(x) {
    TABLE v1, v2 DEPEND c1, c2 FROM -4 TO 6 WITH 800
    v1 = sin(c1 * x) + 2
    v2 = cos(c2 * x) + 2
}
