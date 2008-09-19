: Calcium activated K channel.
: From Moczydlowski and Latorre (1983) J. Gen. Physiol. 82
: Made THREADSAFE 9/16/08

UNITS {
	(molar) = (1/liter)
	(mV) =	(millivolt)
	(mA) =	(milliamp)
	(mM) =	(millimolar)
	FARADAY = (faraday)  (kilocoulombs)
	R = (k-mole) (joule/degC)
}

NEURON {
	SUFFIX cagk
	USEION ca READ cai
	USEION k READ ek WRITE ik
	RANGE gkbar
	GLOBAL oinf, tau
        THREADSAFE : assigned GLOBALs will be per thread
}

PARAMETER {
	celsius		(degC) : 20
	v		(mV)
	gkbar=.01	(mho/cm2)	: Maximum Permeability
	cai		(mM) : 1e-3
	ek		(mV)

	d1 = .84
	d2 = 1.
	k1 = .18	(mM)
	k2 = .011	(mM)
	bbar = .28	(/ms)
	abar = .48	(/ms)
}

ASSIGNED {
	ik		(mA/cm2)
	oinf
	tau		(ms)
}

STATE {	o }		: fraction of open channels

BREAKPOINT {
	SOLVE state METHOD cnexp
	ik = gkbar*o*(v - ek)
}

DERIVATIVE state {
	rate(v, cai)
	o' = (oinf - o)/tau
}

INITIAL {
	rate(v, cai)
	o = oinf
}

FUNCTION alp(v (mV), ca (mM)) (1/ms) { :callable from hoc
	alp = abar/(1 + exp1(k1,d1,v)/ca)
}

FUNCTION bet(v (mV), ca (mM)) (1/ms) { :callable from hoc
	bet = bbar/(1 + ca/exp1(k2,d2,v))
}

FUNCTION exp1(k (mM), d, v (mV)) (mM) { :callable from hoc
	exp1 = k*exp(-2*d*FARADAY*v/R/(273.15 + celsius))
}

PROCEDURE rate(v (mV), ca (mM)) { :callable from hoc
	LOCAL a
	a = alp(v,ca)
	tau = 1/(a + bet(v, ca))
	oinf = a*tau
}
