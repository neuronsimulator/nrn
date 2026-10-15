TITLE minimal POINT_PROCESS with a POINTER for the nrn_pp_setpointer_pop api test

: A point process (not a density mechanism) with a single POINTER (src), a
: settable source value (feed), and a readback (out). INITIAL copies the
: pointed-to value into out, so a test can cross-wire two instances that sit at
: the SAME segment -- each src pointed at the OTHER instance's feed, exactly the
: HalfGap shape -- call finitialize, and read out back to confirm each instance
: read its own wired source. That disambiguation is impossible to address by
: (section, x) alone, which is why point processes need nrn_pp_setpointer_pop
: (object-addressed) rather than nrn_setpointer_pop (segment-addressed).
:
: feed is a PARAMETER so it survives finitialize and is not touched by dynamics,
: giving each instance a stable, distinct source value.

NEURON {
    POINT_PROCESS PPPtr
    POINTER src
    RANGE out, feed
}

PARAMETER {
    feed = 0
}

ASSIGNED {
    src
    out
}

INITIAL {
    out = src
}

BREAKPOINT {
    out = src
}
