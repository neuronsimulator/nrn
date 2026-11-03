NEURON {
    SUFFIX locals
}

PARAMETER {
    a = -1.0
}

FUNCTION get_a() { LOCAL a
    a = 32.0
VERBATIM
    _lget_a = _la;
ENDVERBATIM
}

FUNCTION get_b() { LOCAL a, b
    a = -1.0
    b = 32.0
    { LOCAL a
        a = 100.0
        b = b + a
        VERBATIM
        _lget_b = _lb;
        ENDVERBATIM
    }
}
