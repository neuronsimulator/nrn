: Hodgkin - Huxley k channel

NEURON {
	SUFFIX HHk
	USEION k READ ek WRITE ik
	RANGE gkbar, ik, g
	GLOBAL inf, tau
}

UNITS {
	(mA) = (milliamp)
	(mV) = (millivolt)
}

PARAMETER {
	gkbar= 0.036 (mho/cm2) <0,1e9>
}

STATE {
	n
}

ASSIGNED {
	v (mV)
	ek (mV)
	celsius (degC)
	ik (mA/cm2)
	inf
	tau (ms)
	g (mho/cm2)
}

INITIAL {
	rate(v)
	n = inf
}

BREAKPOINT {
	SOLVE states METHOD cnexp
	g = gkbar*n*n*n*n
	ik = g*(v - ek)
}

DERIVATIVE states {
	rate(v)
	n' =(inf - n)/tau
}

FUNCTION alp(v(mV))(/ms) { LOCAL q10
	v = -v - 65
	q10 = 3^((celsius - 6.3)/10 (degC))
	alp = q10 * 0.01(/ms-mV)*expM1(v + 10, 10(mV))
}

FUNCTION bet(v(mV))(/ms) { LOCAL q10
	v = -v - 65
	q10 = 3^((celsius - 6.3)/10 (degC))
	bet = q10 * 0.125(/ms)*exp(v/80(mV))
}

FUNCTION expM1(x (mV),y (mV)) (mV) {
        if (fabs(x/y) < 1e-6) {
                expM1 = y*(1 - x/y/2)
        }else{
                expM1 = x/(exp(x/y) - 1)
        }
}

PROCEDURE rate(v (mV)) {LOCAL a, b
	TABLE inf, tau DEPEND celsius FROM -100 TO 100 WITH 200
	a = alp(v)  b=bet(v)
	tau = 1/(a + b)
	inf = a/(a + b)
}
