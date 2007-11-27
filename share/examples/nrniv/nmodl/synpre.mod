COMMENT

A synaptic current with alpha function conductance defined by
        i = g * (v - e)      i(nanoamps), g(micromhos);
        where
         g = 0 for t < onset and
         g = gmax * (t - onset)/tau * exp(-(t - onset - tau)/tau)
          for t > onset
this has the property that the maximum value is gmax and occurs at
 t = onset + tau.

The synaptic onset time has been modified to occur when the presynaptic
voltage, vpre, rises above threshold, vprethresh.
Another event is not allowed to occur for, deadtime, milliseconds after
vpre rises above threshold.

The user specifies the presynaptic location in hoc via the statement
	connect vpre_syn[i] = v.section(x)

where x is the arc length (0 - 1) along the presynaptic section (the currently
specified section), and i is the synapse number (Which is located at the
postsynaptic location in the usual way via
	postsynaptic_section {loc_syn(i, x)}
Notice that loc_syn() must be executed first since that function also
allocates space for the synapse.

ENDCOMMENT
					       
NEURON {
	POINT_PROCESS synp
	POINTER vpre
	RANGE onset, tau, gmax, e, i, vprethresh
	NONSPECIFIC_CURRENT i
}
UNITS {
	(nA) = (nanoamp)
	(mV) = (millivolt)
	(umho) = (micromho)
}

PARAMETER {
	tau=.1 (ms)
	gmax=0 	(umho)
	e=0	(mV)
	v	(mV)
	vprethresh (mV)
	onset (ms) : initialize to  < -deadtime
}

PARAMETER {
	deadtime =1 (ms)
}

ASSIGNED {
	i (nA)
	g (umho)
	vpre (mV)
}

INITIAL {
	onset = -1e10
}

BREAKPOINT {
	SOLVE getonset
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

PROCEDURE getonset() {
	:will crash if user hasn't set vpre with the connect statement */

	if (vpre > vprethresh && t > onset + deadtime) {
		onset = t
	}

	VERBATIM
	return 0;
	ENDVERBATIM
}
