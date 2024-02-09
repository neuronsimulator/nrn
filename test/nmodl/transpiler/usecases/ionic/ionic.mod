NEURON {
        SUFFIX ionic
        USEION na READ ina WRITE ena
}

ASSIGNED {
        ina (mA/cm2)
        ena (mV)
}

BREAKPOINT {
        ena = 42.0
}
