NEURON {     
        SUFFIX rchan
        NONSPECIFIC_CURRENT i
        GLOBAL mean, stdev
}

PARAMETER {
        mean = 0
        stdev = .001
}

ASSIGNED {
        i (milliamp/cm2)
        i_ran (milliamp/cm2)
}

BREAKPOINT {
        SOLVE ran
        i = i_ran
}

PROCEDURE ran() {
        i_ran = normrand(mean, stdev)
}

