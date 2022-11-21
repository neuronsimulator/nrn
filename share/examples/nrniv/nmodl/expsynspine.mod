NEURON {
	POINT_PROCESS ExpSynSpine
	RANGE tau, e, i, rsp, vsp
	NONSPECIFIC_CURRENT i
}

UNITS {
	(nA) = (nanoamp)
	(mV) = (millivolt)
	(uS) = (microsiemens)
}

PARAMETER {
	tau = 0.1 (ms) <1e-9,1e9>
	e = 0	(mV)
	rsp = 0 (megohm)
}

ASSIGNED {
	v (mV)
}

STATE {
	g (uS)
	i (nA)
	vsp (mV)        
}

INITIAL {
	g=0
}

BREAKPOINT {
	SOLVE state METHOD cnexp
        linspine()
}

DERIVATIVE state {
	g' = -g/tau
}

PROCEDURE linspine() {
    SOLVE spine
}

LINEAR spine SOLVEFOR i, vsp {
  ~ rsp * i = v - vsp
  ~ i = g*vsp - g*e
}

NET_RECEIVE(weight (uS)) {
	g = g + weight
}
