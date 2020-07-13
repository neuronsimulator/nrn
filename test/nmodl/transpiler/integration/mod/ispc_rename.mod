NEURON {
    SUFFIX test_ispc_rename
    RANGE d1, d2, d3, var_d3, d4
}
ASSIGNED {
    d1
    d2
    d3
    var_d3
    d4
}
INITIAL {
    d1 = 1
    d2 = 2
    d3 = 3
    var_d3 = 3
}
PROCEDURE func () {
VERBATIM
    d4 = 4;
ENDVERBATIM
}
