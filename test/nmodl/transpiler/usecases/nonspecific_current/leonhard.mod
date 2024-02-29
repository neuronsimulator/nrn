UNITS {
        (mA) = (milliamp)
}

NEURON {
        SUFFIX leonhard
        NONSPECIFIC_CURRENT il
}

ASSIGNED {
        il (mA/cm2)
}

BREAKPOINT {
        il = 0.005 * (v - 1.5)
}
