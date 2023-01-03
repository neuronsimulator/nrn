NEURON {
  SUFFIX disc
  GLOBAL k, a0
}

PARAMETER {
  k = 1
}

STATE {
  a
}

INITIAL {
  a = a0
}

BREAKPOINT {
  SOLVE dis
}

DISCRETE dis {
  a = a - k*a@1
}
