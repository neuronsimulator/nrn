NEURON {
    SUFFIX psection_regression
    THREADSAFE
    BBCOREPOINTER rng
}


ASSIGNED {
    rng
    usingR123
}


PROCEDURE setRNG() {
VERBATIM
#ifndef CORENEURON_BUILD
    if( ifarg(1) && hoc_is_double_arg(1) ) {
        nrnran123_State** pv = (nrnran123_State**)(&_p_rng);

        if (*pv) {
            nrnran123_deletestream(*pv);
            *pv = (nrnran123_State*)0;
        }

        *pv = nrnran123_newstream3(1, 2, 3);
        usingR123 = 1;
    }
#endif
ENDVERBATIM
}

VERBATIM
static void bbcore_write(double* x, int* d, int* xx, int* offset, _threadargsproto_) {
  // no-op
}
static void bbcore_read(double* x, int* d, int* xx, int* offset, _threadargsproto_) {
}
ENDVERBATIM
