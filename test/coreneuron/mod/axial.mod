NEURON {
    SUFFIX axial
}

INCLUDE "axial.inc"

: get rid of following when nmodl allows multiple BEFORE STEP in same
:mod file
BEFORE STEP {
    if (ri > 0) {
        pim = pim - ia : child contributions
    }
}

