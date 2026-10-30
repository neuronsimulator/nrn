: POINT_PROCESS BEFORE STEP with a POINTER and a VERBATIM _p_ name.
: p is a double written with setpointer. raw is not: _p_raw is a void* lvalue,
: and literal_value<void*>() rejects a slot that setpointer stored as double*.

NEURON {
    POINT_PROCESS BefstepPP
    POINTER p, raw
    RANGE n, nverb
}

ASSIGNED {
    p
    raw
    n
    nverb
}

INITIAL {
    n = 0
    nverb = 0
}

BEFORE STEP {
    n = n + 1
    p = p + 1
VERBATIM
    {
        void** slot = (void**)(&(_p_raw));
        if (*slot == (void*)0) {
            *slot = (void*)1;
        }
        nverb = nverb + 1.0;
    }
ENDVERBATIM
}
