: Test LAG statement.

NEURON {
    POINT_PROCESS LagV
    RANGE vl, dur
}

UNITS {
    (mV) = (millivolt)
}

PARAMETER {
   dur  = 1 (ms)
}

ASSIGNED {
    v (mV)
    vl (mV)
}

BEFORE STEP {
    LAG v BY dur    : not threadsafe, not good with cvode
    vl = lag_v_dur * 1(mV) : modlunit bug -- not given dimension
}


