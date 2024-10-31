NEURON {
    SUFFIX scalar
}

PARAMETER {
    freq = 10
    a = 5
    v1 = -1
    v2 = 5
    v3 = 15
    v4 = 0.8
    v5 = 0.3
    r = 3
    k = 0.2
}

STATE {var1 var2 var3}

INITIAL {
    var1 = v1
    var2 = v2
    var3 = v3
}

BREAKPOINT {
  SOLVE equation METHOD derivimplicit
}


DERIVATIVE equation {
    : eq with a function on RHS
    var1' = -sin(freq * t)
    : simple ODE (nonzero Jacobian)
    var2' = -var2 * a
    : logistic ODE
    var3' = r * var3 * (1 - var3 / k)
}
