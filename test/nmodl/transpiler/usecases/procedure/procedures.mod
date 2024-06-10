NEURON {
    SUFFIX procedures
    RANGE x
}

ASSIGNED {
    x
}

FUNCTION identity(v) {
    identity = v
}

PROCEDURE set_x_42() {
    set_x_a(42)
}

PROCEDURE set_x_a(a) {
    x = a
}

PROCEDURE set_a_x() {
    LOCAL a
    a = x
}

PROCEDURE set_x_v() {
    x = v
}

PROCEDURE set_x_just_v() {
    x = identity(v)
}

PROCEDURE set_x_just_vv(v) {
    x = identity(v)
}

INITIAL {
    set_a_x()
}
