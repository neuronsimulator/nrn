NEURON {
    SUFFIX func_in_breakpoint
    NONSPECIFIC_CURRENT il
}

ASSIGNED {
    il
}

PARAMETER {
    c = 1
}

PROCEDURE func() { }
PROCEDURE func_with_v(v) { }
PROCEDURE func_with_other(q) { }

BREAKPOINT {
    func()
    func_with_v(v)
    func_with_other(c)
}
