: simple first-order model of calcium dynamics

NEURON {
SUFFIX ican
USEION n READ ni, in WRITE ni
RANGE n
}

ASSIGNED {
in		(mA/cm2)
}

