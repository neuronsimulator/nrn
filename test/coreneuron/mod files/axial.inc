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
    : Parent ia is not a defined input yet, so do not form im from pia.
    : The root has ri == 0 and never executes im = ia in AFTER SOLVE,
    : so this zero is the value it starts from (#3656).
    im = 0
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

BEFORE STEP {
    if (ri > 0) {
        PROTECT pim = pim - ia : child contributions
    }
}
