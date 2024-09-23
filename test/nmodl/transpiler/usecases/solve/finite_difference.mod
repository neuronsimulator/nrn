NEURON {
    SUFFIX finite_difference
    GLOBAL a
    THREADSAFE
}

ASSIGNED {
  a
}

STATE {
  x
}

INITIAL {
    x = 42.0
    a = 0.1
}

BREAKPOINT {
    SOLVE dX METHOD derivimplicit
}

DERIVATIVE dX {
    x' = -f(x)
}

FUNCTION f(x) {
    f = a*x
}
