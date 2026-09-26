NEURON {
    POINT_PROCESS receiver
    RANGE y
}

ASSIGNED {
    y
}

INITIAL {
    y = 0.0
}

NET_RECEIVE(w) {
    y = y + 0.1
}
