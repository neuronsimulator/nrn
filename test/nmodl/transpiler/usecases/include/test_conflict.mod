NEURON {
    SUFFIX test_conflict
}

INCLUDE "test_conflict.inc"

INITIAL {
    LOCAL foo
    foo = 1
}

BREAKPOINT {
    LOCAL bar
    bar = 2
}

