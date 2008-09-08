TITLE HH sodium channel
: Hodgkin - Huxley squid sodium channel

NEURON {
	SUFFIX HHna
	USEION na READ ena WRITE ina
	RANGE gnabar, ina
	GLOBAL minf, hinf, mtau, htau
	THREADSAFE
}

UNITS {
	(mA) = (milliamp)
	(mV) = (millivolt)
}

PARAMETER {
	gnabar=.120 (mho/cm2) <0,1e9>
}

STATE {
	m h
}

ASSIGNED {
	v (mV)
	celsius (degC) : 6.3
	ena (mV)
	ina (mA/cm2)
	minf hinf
	mtau (ms)
	htau (ms)
}

INITIAL {
	rates(v)
	m = minf
	h = hinf
}

BREAKPOINT {
	SOLVE states METHOD cnexp
	ina = gnabar*m*m*m*h*(v - ena)
}

DERIVATIVE states {
	rates(v)
	m' = (minf - m)/mtau
	h' = (hinf - h)/htau
}

FUNCTION alp(v(mV),i) (/ms) { LOCAL a,b,c,q10 :rest = -70  order m,h
	v = -v - 65(mV)  :convert to hh convention
	q10 = 3^((celsius - 6.3(degC))/10(degC))
	if (i==0) {
		alp = q10*.1(/ms)*expM1(v *1(/mV) + 25, 10)
	}else if (i==1){
		alp = q10*.07(/ms)*exp(v/20(mV))
	}
}

FUNCTION bet(v(mV),i)(/ms) { LOCAL a,b,c,q10 :rest = -70  order m,h
	v = -v - 65
	q10 = 3^((celsius - 6.3(degC))/10(degC))
	if (i==0) {
		bet = q10* 4(/ms)*exp(v/18(mV))
	}else if (i==1){
		bet = q10*1(/ms)/(exp(.1(/mV)*v + 3) + 1)
	}
}

FUNCTION expM1(x,y) {
	if (fabs(x/y) < 1e-6) {
		expM1 = y*(1 - x/y/2)
	}else{
		expM1 = x/(exp(x/y) - 1)
	}
}

PROCEDURE rates(v(mV)) {LOCAL a, b
	TABLE minf, hinf, mtau, htau DEPEND celsius FROM -100 TO 100 WITH 200
	a = alp(v,0)  b=bet(v,0)
	mtau = 1/(a + b)
	minf = a/(a + b)
	a = alp(v,1)  b=bet(v,1)
	htau = 1/(a + b)
	hinf = a/(a + b)
}
