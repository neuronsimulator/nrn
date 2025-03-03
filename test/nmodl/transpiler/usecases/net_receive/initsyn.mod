NEURON {
    POINT_PROCESS InitSyn
    RANGE a0
}

ASSIGNED {
    a0
}

NET_RECEIVE(w, a) {
    INITIAL {
        a = a0
    }
}
