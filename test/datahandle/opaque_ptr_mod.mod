NEURON {
    SUFFIX opaque_ptr_mod
    POINTER ptr
}

ASSIGNED {
    ptr
}

INITIAL {
VERBATIM
#if NRN_VERSION_LT(9, 0, 0)
    _p_ptr = (double*) malloc(sizeof(int));
#else
    _p_ptr = (int*) malloc(sizeof(int));
#endif
ENDVERBATIM
}
