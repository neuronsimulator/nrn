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
        foo()
    }
}

AFTER SOLVE {
    if (ri > 0) {
        ia = (pv - v) / ri
        im = ia : contribution from parent side
    }
}

BEFORE STEP {
    if (ri > 0) {
        PROTECT pim = pim - ia : child contributions
    }
}

PROCEDURE foo() { : neuronsimulator/nrn#1782 is fixed
    if (ri > 0) {
        : just for compile test. Although this overwrites im from BEFORE STEP
        : the recording of im takes place before the AFTER INITIAL
        pim = pim + 1
    }
}
