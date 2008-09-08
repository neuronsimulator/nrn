TITLE HH k channel channel
: Hodgkin - Huxley k channel


NEURON {
	SUFFIX HHk
	USEION k READ ek WRITE ik
	RANGE gkbar, ik
	GLOBAL inf, tau
	THREADSAFE
}

UNITS {
	(mA) = (milliamp)
	(mV) = (millivolt)
}

PARAMETER {
	gkbar=.036 (mho/cm2) <0,1e9>
	ek (mV) : -77 suggested, default set by NEURON
}
STATE {
	n
}
ASSIGNED {
	v (mV)
	celsius (degC) : 16
	ik (mA/cm2)
	inf
	tau (ms)
}

INITIAL {
	rate(v*1(/mV))
	n = inf
}

BREAKPOINT {
	SOLVE states METHOD cnexp
	ik = gkbar*n*n*n*n*(v - ek)
}

DERIVATIVE states {	: exact when v held constant
	rate(v*1(/mV))
	n' = (inf - n)/tau
}

UNITSOFF
FUNCTION alp(v(mV)) { LOCAL q10
	v = -v - 65
	q10 = 3^((celsius - 6.3)/10)
	alp = q10 * .01*expM1(v + 10, 10)
}

FUNCTION bet(v(mV)) { LOCAL q10
	v = -v - 65
	q10 = 3^((celsius - 6.3)/10)
	bet = q10 * .125*exp(v/80)
}

FUNCTION expM1(x,y) {
        if (fabs(x/y) < 1e-6) {
                expM1 = y*(1 - x/y/2)
        }else{
                expM1 = x/(exp(x/y) - 1)
        }
}


PROCEDURE rate(v) {LOCAL a, b :rest = -70
	TABLE inf, tau DEPEND celsius FROM -100 TO 100 WITH 200
		a = alp(v)  b=bet(v)
		tau = 1/(a + b)
		inf = a/(a + b)
}
UNITSON
