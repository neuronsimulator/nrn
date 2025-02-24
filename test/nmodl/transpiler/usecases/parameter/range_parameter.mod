NEURON {
    POINT_PROCESS range_parameter
    RANGE x, y
}

PARAMETER {
    x = 42
    y
}

INITIAL {
    y = 43
}
