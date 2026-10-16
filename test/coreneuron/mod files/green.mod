NEURON {
    SUFFIX green
    RANGE tau, upsilon
}

DEFINE M 16

ASSIGNED {
    tau
    upsilon[M]
}


INITIAL {
    FROM i = 0 TO M-1 {
        upsilon[i] = 2.0 + 8*tau
    }
}

BREAKPOINT {
    FROM i = 0 TO M-1 {
        upsilon[i] = upsilon[i] + (i/3.0+1)*dt*cos((i/3.0+1) * t)
    }
}

