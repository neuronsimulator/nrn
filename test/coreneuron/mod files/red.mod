NEURON {
    SUFFIX red
    RANGE tau,  upsilon
}

DEFINE M 16

ASSIGNED {
    tau
    upsilon[M]
}


INITIAL {
    FROM i = 0 TO M-1 {
        upsilon[i] = 0.0
    }
}

BREAKPOINT {
    FROM i = 0 TO M-1 {
        upsilon[i] = sin((i + 1.0) * t)
    }
}

