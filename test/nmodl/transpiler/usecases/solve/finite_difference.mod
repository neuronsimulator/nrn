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
  z
}

INITIAL {
    x = 42.0
    z = 21.0
    a = 0.1
}

BREAKPOINT {
    SOLVE dX METHOD derivimplicit
}

DERIVATIVE dX {
    x' = f(x)
    z' = 2.0*f(z)
}

FUNCTION f(x) {
    f = -a*x
}
