NEURON {
    POINT_PROCESS toggle
    RANGE y
}

ASSIGNED {
    y
}

INITIAL {
    y = 0
    net_send(2.001, 1)
}

NET_RECEIVE(w) {
    y = y + 1.0

    if(t < 3.7) {
        net_send(4.001 - t, 1)
    }
}
