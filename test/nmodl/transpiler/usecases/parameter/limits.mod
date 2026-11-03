: Only test if all of these result in compilable code.

NEURON {
    SUFFIX limits_mod
}

PARAMETER {
    a = 24. (mV) <-2, 3>
    b = 24. (mV) <-2.1, 3>
    c = 24. (mV) <-4.1,-3>
    x = 24. (mV) < -2, 3>
    y = 24. (mV) < -2.1, 3>
    z = 24. (mV) < -4.1, -3>
}
