: No more segfault for NET_RECEIVE INITIAL references a RANGE
: variable when translated as not vectorized

: Note that assigning a value to a RANGE variable is a bit of an
: abuse of the NET_RECEIVE INITIAL block since that is intended only to
: initialize arguments (after the first) of the NET_RECEIVE block, i.e.
: NetCon.weight[i] for i > 0. Such RANGE variables are properly
: initialized in the top level INITIAL block.

NEURON {
    POINT_PROCESS NetRecInit
    RANGE foo
    POINTER ptr : without THREADSAFE, this forces NOT VECTORIZED
}

PARAMETER {
    foo = 1
}

ASSIGNED {
    ptr : to avoid error, must be set to a reference in the interpreter
}

NET_RECEIVE(w, a, b) {
    INITIAL {
        a = foo
        b = ptr
    }
}
