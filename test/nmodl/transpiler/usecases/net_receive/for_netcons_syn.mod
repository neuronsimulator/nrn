NEURON {
    POINT_PROCESS ForNetconsSyn
    RANGE a0
}

ASSIGNED {
    a0
}

NET_RECEIVE(w, a) {
    INITIAL {
        a = a0
    }

    FOR_NETCONS(wi, ai) {
        ai = 2.0*ai
    }
}
