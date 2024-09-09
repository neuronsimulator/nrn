NEURON {
    POINT_PROCESS point_non_threadsafe
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

FUNCTION v_plus_a(a) {
    v_plus_a = v + a
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

