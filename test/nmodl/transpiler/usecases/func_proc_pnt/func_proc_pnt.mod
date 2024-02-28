NEURON {
    POINT_PROCESS test_func_proc_pnt
    RANGE x
}

ASSIGNED {
    x
}

PROCEDURE set_x_42() {
    x = 42
}

PROCEDURE set_x_a(a) {
    x = a
}

FUNCTION x_plus_a(a) {
    x_plus_a = x + a
}
