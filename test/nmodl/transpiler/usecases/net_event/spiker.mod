NEURON {
    POINT_PROCESS spiker
    RANGE tnext
}

ASSIGNED {
    tnext
}

INITIAL {
    tnext = 1.001
}

NET_RECEIVE(w) {
    LOCAL tt
    tt = tnext
    if(t >= tt) {
        net_event(t)
        tnext = tnext + 1.0
    }
}
