NEURON {
    SUFFIX non_threadsafe
    GLOBAL gbl
}

LOCAL top_local

PARAMETER {
    parameter = 41.0
}

ASSIGNED {
    gbl
}

FUNCTION get_gbl() {
    get_gbl = gbl
}

FUNCTION get_top_local() {
    get_top_local = top_local
}

FUNCTION get_parameter() {
    get_parameter = parameter
}

INITIAL {
    gbl = 42.0
    top_local = 43.0
}

: A LINEAR block makes the MOD file not thread-safe and not
: vectorized. We don't otherwise care about anything below
: this comment.
STATE {
  z
}

LINEAR lin {
  ~ z = 2
}

