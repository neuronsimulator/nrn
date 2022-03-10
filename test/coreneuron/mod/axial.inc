: Axial current and derivative with respect to x
: INCLUDE in density and point process

NEURON {
    THREADSAFE
    RANGE ri, ia, im
    POINTER pv, pia, pim
}

UNITS {
  (mv) = (millivolt)
  (nA) = (nanoamp)
  (MOhm) = (megohm)
}

PARAMETER {
    ri (MOhm) : center of compartment to center of parent compartment
}

ASSIGNED {
    v (mV)
    ia (nA)
    im (nA) : im - i_membrane_ = electode_currents
    pv (mV) : parent v
    pia (nA) : parent ia 
    pim (nA) : parent im
}

AFTER INITIAL {
    im = 0 : pia not ready yet
    if (ri > 0) {
        ia = (pv - v) / ri
    }
}

AFTER SOLVE {
    if (ri > 0) {
        ia = (pv - v) / ri
        im = ia : contribution from parent side
    }
}

COMMENT
Need to explictly duplicate in axial.mod and axial_pp.mod since
a test of coreneuron checkpoint requires calculating iaSum and
nmodl does not currently allow multiple BEFORE STEP in same mod file
BEFORE STEP {
    if (ri > 0) {
        pim = pim - ia : child contributions
    }
}
ENDCOMMENT
