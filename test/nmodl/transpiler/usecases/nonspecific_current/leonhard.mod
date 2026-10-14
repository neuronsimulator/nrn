UNITS {
    (mA) = (milliamp)
}

NEURON {
    SUFFIX leonhard
    NONSPECIFIC_CURRENT il
    RANGE c
}

ASSIGNED {
    il (mA/cm2)
}

PARAMETER {
    c = 0.005
}

BREAKPOINT {
    il = c * (v - 1.5)
}
