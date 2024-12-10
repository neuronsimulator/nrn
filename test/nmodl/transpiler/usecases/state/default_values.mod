NEURON {
    SUFFIX default_values
    RANGE Z, B
    GLOBAL X0, Z0, A0, B0
}

STATE {
    X
    Y
    Z
    A[3]
    B[2]
}

PARAMETER {
    X0 = 2.0
    Z0 = 3.0
    A0 = 4.0
    B0 = 5.0
}

INITIAL {
    Z = 7.0
    B[1] = 8.0
}
