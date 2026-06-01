NEURON {
    POINT_PROCESS NetReceiveCalls
    RANGE c1, c2
}

ASSIGNED {
    c1
    c2
}

INITIAL {
    c1 = 0
    c2 = 0
}

FUNCTION one() {
    one = 1
}

PROCEDURE increment_c2() {
    c2 = c2 + 2
}

NET_RECEIVE(w) {
    c1 = c1 + one()
    increment_c2()
}
