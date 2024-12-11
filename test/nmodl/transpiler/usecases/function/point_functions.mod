NEURON {
    POINT_PROCESS point_functions
    RANGE x
}

ASSIGNED {
    x
}

FUNCTION x_plus_a(a) {
    x_plus_a = x + a
}

FUNCTION v_plus_a(a) {
    v_plus_a = v + a
}

FUNCTION identity(v) {
    identity = v
}

INITIAL {
    x = 1.0
}
