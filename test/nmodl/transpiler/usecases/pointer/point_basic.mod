: POINTER variables are part of the `dparam` array. They're also "known" to
: NEURON and it recomputes their index into `dparam`. Naturally, the two don't
: have to agree, but if they don't it's a bug.
:
: These tests check that the index in MOD files agrees with the index used
: inside of NEURON.

NEURON {
    POINT_PROCESS point_basic
    RANGE x1, x2, ignore
    : The ion pointers immediately preceed the POINTER variables
    : in the dparam array.
    USEION ca READ ica
    POINTER p1, p2
}

ASSIGNED {
    x1
    x2
    p1
    p2
    ica
    ignore
}

INITIAL {
    ignore = ica
    x1 = 0.0
    x2 = 0.0
}

FUNCTION read_p1() {
    read_p1 = p1
}

FUNCTION read_p2() {
    read_p2 = p2
}
