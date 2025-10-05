: Example of MOD file using matexp solver

NEURON {
    SUFFIX glia
    RANGE Aalfa, Valfa, Abeta, Vbeta
}

PARAMETER {
    Aalfa = 353.91 ( /ms)
    Valfa = 13.99 ( /mV)
    Abeta = 1.272  ( /ms)
    Vbeta = 13.99 ( /mV)
    n1 = 5.422
    n4 = 0.738
}

STATE {
    C1
    C2
}

BREAKPOINT {
    SOLVE kstates METHOD matexp
}

FUNCTION alfa(v(mV)) {
    alfa = Aalfa*exp(v/Valfa)
}

FUNCTION beta(v(mV)) {
    beta = Abeta*exp(-v/Vbeta)
}

KINETIC kstates {
    ~ C1 <-> C2 (n1*alfa(v),n4*beta(v))
}

