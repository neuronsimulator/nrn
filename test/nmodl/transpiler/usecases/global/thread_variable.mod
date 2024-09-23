NEURON {
    SUFFIX shared_global
    NONSPECIFIC_CURRENT il
    RANGE y, z
    GLOBAL g_v1, g_w, g_arr
    THREADSAFE
}

ASSIGNED {
    y
    z
    g_w
    g_arr[3]
    il
}

INITIAL {
    g_w = 48.0
    g_v1 = 0.0
    g_arr[0] = 10.0 + z
    g_arr[1] = 10.1
    g_arr[2] = 10.2
    y = 10.0
}

BREAKPOINT {
    if(t > 0.33) {
        g_w = sum_arr()
    }

    if(t > 0.66) {
        set_g_w(z)
        compute_g_v1(z)
    }

    y = g_w + g_v1
    il = 0.0000001 * (v - 10.0)
}

FUNCTION sum_arr() {
    sum_arr = g_arr[0] + g_arr[1] + g_arr[2]
}

PROCEDURE set_g_w(zz) {
    g_w = zz
}

PROCEDURE compute_g_v1(zz) {
    TABLE g_v1 FROM 3 TO 4 WITH 8
    g_v1 = zz * zz
}
