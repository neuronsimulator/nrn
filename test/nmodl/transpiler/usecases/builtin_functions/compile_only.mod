NEURON {
    SUFFIX compile_only
}

FUNCTION call_nrn_ghk() {
    call_nrn_ghk = nrn_ghk(1.0, 2.0, 3.0, 4.0)
}
