NEURON {
    SUFFIX leonhard
    RANGE z
}

ASSIGNED {
  z[3]
}

STATE {
  x
  s[2]
}

INITIAL {
    x = 42.0
    s[0] = 0.1
    s[1] = -1.0
    z[0] = 0.7
    z[1] = 0.8
    z[2] = 0.9
}

BREAKPOINT {
    SOLVE dX METHOD cnexp
}

DERIVATIVE dX {
    x' = (s[0] + s[1])*(z[0]*z[1]*z[2])*x
}
