NEURON {
    POINT_PROCESS spiker
    RANGE y, z
}

ASSIGNED {
    y
    z
}

INITIAL {
    y = 0.0
    z = 0.0
    net_send(1.8, 1)
}

NET_RECEIVE(w) {
    if(flag == 0) {
        y = y + 1
        net_move(t + 0.1)
    } else {
        z = z + 1
        net_send(2.0, 1)
    }
}
