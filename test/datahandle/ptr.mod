NEURON {
    POINT_PROCESS Pnt
    POINTER p
}

ASSIGNED {
    p
}

PROCEDURE pr() {
    printf("*p = %g\n", p)
}


