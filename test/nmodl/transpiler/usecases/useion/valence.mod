NEURON {
    SUFFIX valence_mod
    USEION K READ Ki VALENCE 222
    RANGE x
}

ASSIGNED {
    x
    Ki
}

INITIAL {
    x = Ki
}
