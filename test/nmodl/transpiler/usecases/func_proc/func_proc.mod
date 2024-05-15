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

FUNCTION v_plus_a(a) {
    v_plus_a = v + a
}

PROCEDURE set_x_v() {
    x = v
}

FUNCTION just_v(v) {
    just_v = v
}

PROCEDURE set_x_just_v() {
    x = just_v(v)
}

PROCEDURE set_x_just_vv(v) {
    x = just_v(v)
}

INITIAL {
    set_a_x()
}
