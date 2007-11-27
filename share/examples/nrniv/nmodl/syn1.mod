: this model is built-in to neuron with suffix syn

COMMENT
an synaptic current with alpha function conductance defined by
        i = g * (v - e)      i(nanoamps), g(micromhos);
        where
         g = 0 for t < onset and
         g = gmax * (t - onset)/tau * exp(-(t - onset - tau)/tau)
          for t > onset
this has the property that the maximum value is gmax and occurs at
 t = delay + tau.
ENDCOMMENT
					       
NEURON {
	POINT_PROCESS syn1
	RANGE onset, tau, gmax, e, i
	NONSPECIFIC_CURRENT i
}
UNITS {
	(nA) = (nanoamp)
	(mV) = (millivolt)
	(umho) = (micromho)
}

PARAMETER {
	onset=0 (ms)
	tau=.1 (ms)
	gmax=0 	(umho)
	e=0	(mV)
	v	(mV)
}

ASSIGNED { i (nA)  g (umho)}

BREAKPOINT {
	g = gmax * alpha( (t - onset)/tau )
	i = g*(v - e)
}

FUNCTION alpha(x) {
	if (x < 0 || x > 10) {
		alpha = 0
	}else{
		alpha = x * exp(1 - x)
	}
}
