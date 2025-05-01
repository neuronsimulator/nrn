NEURON {
    SUFFIX nothing
}

FUNCTION forty_two_plus_x(x) {
    forty_two_plus_x = 42.0 + x
}

FUNCTION twice_forty_two_plus_x(x) {
    twice_forty_two_plus_x = 2.0 * forty_two_plus_x(x)
}
