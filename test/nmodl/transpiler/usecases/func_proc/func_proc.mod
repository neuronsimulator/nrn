NEURON {
    SUFFIX test_func_proc
    RANGE x
}

ASSIGNED {
    x
}

PROCEDURE set_x_42() {
    set_x_a(42)
}

PROCEDURE set_x_a(a) {
    x = a
}

FUNCTION x_plus_a(a) {
    x_plus_a = x + a
}

PROCEDURE set_a_x() {
    LOCAL a
    a = x
}
