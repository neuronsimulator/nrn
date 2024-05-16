NEURON {
    POINT_PROCESS test_parameter
    RANGE x, y
}

PARAMETER {
    x = 42
    y
}

INITIAL {
    y = 43
}
