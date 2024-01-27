NEURON {
        SUFFIX pump 
        USEION x READ xi, xo WRITE ix VALENCE 1
        NONSPECIFIC_CURRENT i
        RANGE ix, rate
}

UNITS {
	(mV)	= (millivolt)
	(molar) = (1/liter)
	(mM)	= (millimolar)
	(mA)	= (milliamp)
}


PARAMETER {
    imax = 1    (mA/cm2)
    Kd = 1      (mM)
}

 
ASSIGNED {
    xo      (mM)
    xi      (mM)
    ix      (mA/cm2)
    i       (mA/cm2)
}
 
BREAKPOINT {
    ix = imax * ((xi/(Kd + xi)) - (xo/(Kd + xo))) 
    i = 0  
}
