TITLE minimal POINTER density mechanism for the nrn_setpointer_pop api test

: A density mechanism with a single POINTER (src) and a plain range variable
: (out). INITIAL copies the pointed-to value into out, so a test can wire src
: with nrn_setpointer_pop, call finitialize, and read out back to confirm the
: POINTER now aliases the source's storage.

NEURON {
    SUFFIX ptrtest
    POINTER src
    RANGE out
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
