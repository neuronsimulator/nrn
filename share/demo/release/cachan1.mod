TITLE ca channel
: Ca channel used  by Augustine (1990) J. Physiol. 431
:  341-364   should use at celsius = 16

NEURON {
	SUFFIX cachan1
	USEION ca READ cao,cai WRITE ica
	RANGE K, ica, imax
	GLOBAL inf, tau, rfac
	THREADSAFE
}

UNITS {
	(mA) = (milliamp)
	(mV) = (millivolt)
	(M) = (1/liter)
	(mM) = (milliM)
	k = (k) (joule/degC)
	e = (e) (coulombs)
}

PARAMETER {
	rfac = 1	<.01, 1e5>
	K = 140 (/M)	<1e-6, 1e6>
	imax = 1 (mA/cm2) <0, 1e9>
}

STATE {
	S
}

ASSIGNED {
	v (mV)
	cao (mM)
	cai (mM)
	celsius (degC) : 16
	ica (mA/cm2)
	inf
	tau (ms)
}


INITIAL {
	rate(v)
	S = inf
}

BREAKPOINT {LOCAL kca
	SOLVE states METHOD cnexp
	kca = K*cao*exp(-.08(/mV)*(v+70))
	ica = -imax * S^5 * (kca - K*cai)/(1 + kca)
}

DERIVATIVE states {	: exact when v held constant
	rate(v)
	S' = (inf - S)/tau
}

FUNCTION alp(v(mV))(/ms) {
	alp = rfac * 1.1(/ms)*exp(.85*(.001)*e/k/(273+celsius)*v)
}

FUNCTION bet(v(mV))(/ms) {
		bet = rfac * .168(/ms)*exp(-.38*(.001)*e/k/(273+celsius)*v)
}

PROCEDURE rate(v(mV)) {LOCAL a, b
	TABLE inf, tau DEPEND celsius, rfac FROM -100 TO 100 WITH 200
		a = alp(v)  b=bet(v)
		tau = 1/(a + b)
		inf = a/(a + b)
}
