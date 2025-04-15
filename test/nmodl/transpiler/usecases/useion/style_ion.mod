: This checks for a code-generation bug that resulted in a faulty ctor
: call. It requires an ion to have a "style", followed by other ions.

NEURON {
    SUFFIX style_ion
    USEION ca WRITE cai, eca
    USEION na READ nai
}

ASSIGNED {
    cai
    eca
    nai
}

INITIAL {
    cai = 42.0
}
