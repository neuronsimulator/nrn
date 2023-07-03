: Calcium activated K channel.
: From Moczydlowski and Latorre (1983) J. Gen. Physiol. 82
: Made THREADSAFE 9/16/08

: Use 2-D FUNCTION_TABLE. See cagkftab.py for parameters of the tables
: Also test 1-D FUNCTION_TABLE

UNITS {
	(molar) = (1/liter)
	(mV) =	(millivolt)
	(mA) =	(milliamp)
	(mM) =	(millimolar)
	FARADAY = (faraday)  (kilocoulombs)
	R = (k-mole) (joule/degC)
}

NEURON {
	SUFFIX cagkftab
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

FUNCTION_TABLE alp(v (mV), lnCa) (/ms)
FUNCTION_TABLE bet(v (mV), lnCa) (/ms)

PROCEDURE rate(v (mV), ca (mM)) { :callable from hoc
	LOCAL a, lnCa
        lnCa = log(ca*1(/mM))
	a = alp(v,lnCa)
	tau = 1/(a + bet(v, lnCa))
	oinf = a*tau
}

FUNCTION_TABLE tst(v (mV)) (1)

FUNCTION tst1(v (mV)) (1) {
    tst1 = tst(v)
}

