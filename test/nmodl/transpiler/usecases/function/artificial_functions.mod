NEURON {
    ARTIFICIAL_CELL art_functions
    RANGE x
    GLOBAL gbl
}

ASSIGNED {
    gbl
    v
    x
}

FUNCTION x_plus_a(a) {
    x_plus_a = x + a
}

FUNCTION identity(v) {
    identity = v
}

INITIAL {
    x = 1.0
    gbl = 42.0
}

: A LINEAR block makes a MOD file not VECTORIZED.
STATE {
  z
}

LINEAR lin {
  ~ z = 2
}

