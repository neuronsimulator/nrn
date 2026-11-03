NEURON {
    ARTIFICIAL_CELL art_toggle
    RANGE y
}

ASSIGNED {
    y
}

INITIAL {
    y = 0
    net_send(2.501, 1)
}

NET_RECEIVE(w) {
    y = y + 1.0

    if(t < 3.7) {
        net_send(4.501 - t, 1)
    }
}
