: Density BEFORE/AFTER counters.
: RANGE values start at 0. Do not assign them in INITIAL: BEFORE INITIAL
: runs first and that assignment would wipe the count.

NEURON {
    SUFFIX befstep
    RANGE nbi, nai, nbb, nas, nstep, vsave, tsave
}

ASSIGNED {
    nbi
    nai
    nbb
    nas
    nstep
    vsave
    tsave
}

PROCEDURE add_nstep() {
    nstep = nstep + 1
    : A return keeps this call from being inlined, so the generated
    : callback has to pass the per-instance arguments.
    VERBATIM
    return ret_add_nstep;
    ENDVERBATIM
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
    add_nstep()
    vsave = v
    tsave = t
}
