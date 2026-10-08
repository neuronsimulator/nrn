NEURON {
    SUFFIX shared_counter
    GLOBAL g_cnt
}

ASSIGNED {
    g_cnt
}


INITIAL {
    PROTECT g_cnt = 0
}

BREAKPOINT {
    PROTECT g_cnt = g_cnt + 1
}
