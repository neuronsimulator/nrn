NEURON {
    SUFFIX NeuronVariables
    RANGE range_celsius
}

PARAMETER {
    celsius = 123.456
}

ASSIGNED {
    range_celsius
}

BREAKPOINT {
    range_celsius = celsius
}
