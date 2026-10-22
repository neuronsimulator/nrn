: A point process that writes a calcium current, for test_pp_ion.py: instances that share a node
: all add to that node's ica.
NEURON {
    POINT_PROCESS CaWriterPP
    USEION ca WRITE ica
    RANGE amp
}

UNITS {
    (nA) = (nanoamp)
}

PARAMETER {
    amp = 0.001 (nA)
}

ASSIGNED {
    ica (nA)
}

BREAKPOINT {
    ica = amp
}
