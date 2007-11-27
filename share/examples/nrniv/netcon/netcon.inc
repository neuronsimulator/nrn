COMMENT
USAGE: for most receptors
*****************************************************************************
    NEURON {
      POINT_PROCESS NAME
    }

    PARAMETER {
      Cdur	= 1.08	(ms)		: transmitter duration (rising phase)
      Alpha	= 1	(/ms mM)	: forward (binding) rate
      Beta	= 0.02	(/ms)		: backward (unbinding) rate
      Erev	= -80	(mV)		: reversal potential
    }
    
    INCLUDE "netsyn.inc"
*****************************************************************************

USAGE: for NMDA receptor
*****************************************************************************
    NEURON{ POINT_PROCESS NMDA
      RANGE B 
    }

    PARAMETER {
      mg        = 1.    (mM)     : external magnesium concentration
      Cdur	= 1.	(ms)	 : transmitter duration (rising phase)
      Alpha	= 4.	(/ms mM) : forward (binding) rate
      Beta	= 0.0067 (/ms)	 : backward (unbinding) rate 1/150
      Erev	= 0.	(mV)	 : reversal potential
    }

    ASSIGNED { B }

    INCLUDE "netcon.inc"
    : EXTRA BREAKPOINT MUST BE BELOW THE INCLUDE
    BREAKPOINT {
      rates(v)
      g = g * B : but don't really need to readjust conductance
      i = i * B : i = g*(v - Erev)
    }

    PROCEDURE rates(v(mV)) {
      TABLE B
      DEPEND mg
      FROM -100 TO 80 WITH 180
      B = 1 / (1 + exp(0.062 (/mV) * -v) * (mg / 3.57 (mM)))
    }
*****************************************************************************
ENDCOMMENT

INDEPENDENT {t FROM 0 TO 1 WITH 1 (ms)}

NEURON {
	RANGE g
	NONSPECIFIC_CURRENT i
	GLOBAL Cdur, Alpha, Beta, Erev, Rinf, Rtau
}

UNITS {
	(nA) = (nanoamp)
	(mV) = (millivolt)
	(umho) = (micromho)
	(mM) = (milli/liter)
}

ASSIGNED {
	v		(mV)		: postsynaptic voltage
	i 		(nA)		: current = g*(v - Erev)
	g 		(umho)		: conductance
	Rinf				: steady state channels open
	Rtau		(ms)		: time constant of channel binding
	R				: fraction of open channels
	synon
}

STATE {Ron Roff}

INITIAL {
	R = 0
	Rinf = Alpha / (Alpha + Beta)
	Rtau = 1 / (Alpha + Beta)
	synon = 0
}

BREAKPOINT {
	SOLVE release METHOD cnexp
	g = (Ron + Roff)*1(umho)
	i = g*(v - Erev)
}

DERIVATIVE release {
	Ron' = (synon*Rinf - Ron)/Rtau
	Roff' = -Beta*Roff
}

: following supports both saturation from single input and
: summation from multiple inputs
: if spike occurs during CDur then new off time is t + CDur
: ie. transmitter concatenates but does not summate
: Note: automatic initialization of all reference args to 0 except first

NET_RECEIVE(weight, on, nspike, r0, t0 (ms)) {
	: flag is an implicit argument of NET_RECEIVE and  normally 0
        if (flag == 0) { : a spike, so turn on if not already in a Cdur pulse
		nspike = nspike + 1
		if (!on) {
			r0 = r0*exp(-Beta*(t - t0))
			t0 = t
			on = 1
			synon = synon + weight
			state_discontinuity(Ron, Ron + r0)
			state_discontinuity(Roff, Roff - r0)
		}
		: come again in Cdur with flag = current value of nspike
		net_send(Cdur, nspike)
        }
	if (flag == nspike) { : if this associated with last spike then turn off
		r0 = weight*Rinf + (r0 - weight*Rinf)*exp(-(t - t0)/Rtau)
		t0 = t
		synon = synon - weight
		state_discontinuity(Ron, Ron - r0)
		state_discontinuity(Roff, Roff + r0)
		on = 0
	}
}
