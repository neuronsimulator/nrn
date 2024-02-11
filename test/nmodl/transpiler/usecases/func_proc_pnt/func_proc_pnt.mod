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

FUNCTION get_a_42(a) {
    get_a_42 = a + 42
}
