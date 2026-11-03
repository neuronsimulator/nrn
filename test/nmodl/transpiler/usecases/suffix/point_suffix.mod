: The name of this mod file should be `point_suffix`.
NEURON {
    POINT_PROCESS point_suffix
    RANGE x
}

ASSIGNED {
    x
}

INITIAL {
    x = 42
}
