: Density BEFORE/AFTER counters.
: RANGE values start at 0. Do not assign them in INITIAL: BEFORE INITIAL
: runs first and that assignment would wipe the count.

NEURON {
    SUFFIX befstep
    RANGE nbi, nai, nbb, nas, nstep, vsave, tsave
}

ASSIGNED {
    : nocmodl requires this declaration. v is the node voltage, not a RANGE slot.
    v (millivolt)
    nbi
    nai
    nbb
    nas
    nstep
    vsave
    tsave
}

BEFORE INITIAL {
    nbi = nbi + 1
}

AFTER INITIAL {
    nai = nai + 1
}

BEFORE BREAKPOINT {
    nbb = nbb + 1
}

AFTER SOLVE {
    nas = nas + 1
}

BEFORE STEP {
    : Direct assignment. A PROCEDURE that contains VERBATIM is not thread
    : safe under nocmodl, so the call would miss this callback's cache.
    nstep = nstep + 1
    vsave = v
    tsave = t
}
