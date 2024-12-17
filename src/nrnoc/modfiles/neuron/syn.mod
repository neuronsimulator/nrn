COMMENT
an synaptic current with alpha function conductance defined by
        i = g * (v - e)      i(nanoamps), g(microsiemens);
        where
         g = 0 for t < onset and
         g = gmax * (t - onset)/tau * exp(-(t - onset - tau)/tau)
          for t > onset
this has the property that the maximum value is gmax and occurs at
 t = delay + tau.
ENDCOMMENT
					       
NEURON {
	POINT_PROCESS AlphaSynapse
	RANGE onset, tau, gmax, e, i
	NONSPECIFIC_CURRENT i
}
UNITS {
	(nA) = (nanoamp)
	(mV) = (millivolt)
	(uS) = (microsiemens)
}

PARAMETER {
	onset=0 (ms)
	tau=.1 (ms)	<1e-3,1e6>
	gmax=0 	(uS)	<0,1e9>
	e=0	(mV)
}

ASSIGNED { v (mV) i (nA)  g (uS)}

BREAKPOINT {
	if (gmax) { at_time(onset) }
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
