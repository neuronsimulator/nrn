NEURON {
    SUFFIX heat_eqn_array
    RANGE x
}

DEFINE N  4

PARAMETER {
    kf = 0.0
    kb = 0.0
}

ASSIGNED {
    x
    mu[N]
    vol[N]
}

STATE {
    X[N]
}

INITIAL {
    FROM i=0 TO N-1 {
        mu[i] = 1.0 + i
        vol[i] = 0.01 / (i + 1.0)

        if(x < 0.5) {
            X[i] = 1.0 + i
        } else {
            X[i] = 0.0
        }
    }
}

BREAKPOINT {
    SOLVE state METHOD sparse
}

KINETIC state {
    COMPARTMENT i, vol[i] {X}
    LONGITUDINAL_DIFFUSION i, mu[i] {X}

    FROM i=0 TO N-2 {
        ~ X[i] <-> X[i+1] (kf, kb)
    }
}
