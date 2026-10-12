: Correctness of test_axial depends on nrnivmodl registering axial1.mod
: before axial2.mod. This density AFTER INITIAL must run first (#3656).

NEURON {
    SUFFIX axial
}

INCLUDE "axial.inc"
