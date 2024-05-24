NEURON {
  ARTIFICIAL_CELL art_toggle
  RANGE y
}

PARAMETER {
  y = 0.0
}

INITIAL {
  y = 0
  net_send(1.5, 1)
}

NET_RECEIVE(w) {
  y = 1.0
}
