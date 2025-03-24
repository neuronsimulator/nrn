NEURON {
    SUFFIX two_radii
    NONSPECIFIC_CURRENT il
    RANGE inv
}

ASSIGNED {
    il
    inv
    diam
    area
}

INITIAL {
    inv = 1.0 / (square_diam() + area)
}

BREAKPOINT {
    il = (square_diam() + area) * 0.001 * (v - 20.0)
}

FUNCTION square_diam() {
    square_diam = diam * diam
}

FUNCTION square_area() {
    square_area = area * area
}
