NEURON {
    SUFFIX lin
}

STATE {
  xx
  yy
}

PARAMETER {
    a = 2.0
    b = 3.0
    c = 4.0
    d = 5.0
}


INITIAL {
    SOLVE system
}

LINEAR system {
    ~ a*xx + b*yy = 0
    ~ c*xx + d*yy = 0
}

