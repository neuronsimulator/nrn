NEURON {
    SUFFIX localize_arguments
    USEION na READ ina, nai
    RANGE x
    GLOBAL g
}

ASSIGNED {
    x
    g
    ina
    nai
}

STATE {
    s
}

PARAMETER {
    p = 42.0
}

FUNCTION id_v(v) {
    id_v = v
}

FUNCTION id_nai(nai) {
    id_nai = nai
}

FUNCTION id_ina(ina) {
    id_ina = ina
}

FUNCTION id_x(x) {
    id_x = x
}

FUNCTION id_g(g) {
    id_g = g
}

FUNCTION id_s(s) {
    id_s = s
}

FUNCTION id_p(p) {
    id_p = p
}

: The name of top-LOCAL variables can't be used as
: a function argument.
: FUNCTION id_l(l) {
:     id_l = l
: }

INITIAL {
    x = 42.0
    s = 42.0
}
