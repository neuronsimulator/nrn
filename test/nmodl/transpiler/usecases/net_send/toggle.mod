NEURON {
  POINT_PROCESS toggle
  RANGE y
}

UNITS {
}

ASSIGNED {
  y
}

INITIAL {
  y = 0
  net_send(2.0, 1)
}

NET_RECEIVE(w) {
  y = 1
}
