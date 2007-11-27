TITLE FH channel
: Frankenhaeuser - Huxley channels for Xenopus

NEURON {
	SUFFIX fh
	USEION na READ nai, nao WRITE ina
	USEION k READ ki, ko WRITE ik
	NONSPECIFIC_CURRENT il, ip
	RANGE pnabar, pkbar, ppbar, gl, el, il, ip
	GLOBAL inf,tau
}

INCLUDE "standard.inc"
PARAMETER {
	v (mV)
	celsius (degC) : 20
	pnabar=8e-3 (cm/s)
	ppbar=.54e-3 (cm/s)
	pkbar=1.2e-3 (cm/s)
	nai (mM) : 13.74
	nao (mM) : 114.5
	ki (mM) : 120
	ko (mM) : 2.5
	gl=30.3e-3 (mho/cm2)
	el = -69.74 (mV)
}
STATE {
	m h n p
}
ASSIGNED {
	ina (mA/cm2)
	ik (mA/cm2)
	ip (mA/cm2)
	il (mA/cm2)
	inf[4]
	tau[4] (ms)
}

INITIAL {
	mhnp(v*1(/mV))
	m = inf[0]
	h = inf[1]
	n = inf[2]
	p = inf[3]
}

BREAKPOINT {
	LOCAL ghkna
	SOLVE states METHOD cnexp
	ghkna = ghk(v, nai, nao)
	ina = pnabar*m*m*h*ghkna
	ip = ppbar*p*p*ghkna
	ik = pkbar*n*n*ghk(v, ki, ko)
	il = gl*(v - el)
}

FUNCTION ghk(v(mV), ci(mM), co(mM)) (.001 coul/cm3) {
	:assume a single charge
	LOCAL z, eci, eco
	z = (1e-3)*FARADAY*v/(R*(celsius+273.15))
	eco = co*efun(z)
	eci = ci*efun(-z)
	ghk = (.001)*FARADAY*(eci - eco)
}

FUNCTION efun(z) {
	if (fabs(z) < 1e-4) {
		efun = 1 - z/2
	}else{
		efun = z/(exp(z) - 1)
	}
}

DERIVATIVE states {	: exact when v held constant
	mhnp(v*1(/mV))
	m' = (inf[0] - m)/tau[0]
	h' = (inf[1] - h)/tau[1]
	n' = (inf[2] - n)/tau[2]
	p' = (inf[3] - p)/tau[3]
}

UNITSOFF
FUNCTION alp(v(mV),i) { LOCAL a,b,c,q10 :rest = -70  order m,h,n,p
	v = v+70
	q10 = 3^((celsius - 20)/10)
	if (i==0) {
		a=.36 b=22. c=3.
		alp = q10*a*expM1(b - v, c)
	}else if (i==1){
		a=.1 b=-10. c=6.
		alp = q10*a*expM1(v - b, c)
	}else if (i==2){
		a=.02 b= 35. c=10.
		alp = q10*a*expM1(b - v, c)
	}else{
		a=.006 b= 40. c=10.
		alp = q10*a*expM1(b - v , c)
	}
}

FUNCTION bet(v,i) { LOCAL a,b,c,q10 :rest = -70  order m,h,n,p
	v = v+70
	q10 = 3^((celsius - 20)/10)
	if (i==0) {
		a=.4  b= 13.  c=20.
		bet = q10*a*expM1(v - b, c)
	}else if (i==1){
		a=4.5  b= 45.  c=10.
		bet = q10*a/(exp((b - v)/c) + 1)
	}else if (i==2){
		a=.05  b= 10.  c=10.
		bet = q10*a*expM1(v - b, c)
	}else{
		a=.09 b= -25. c=20.
		bet = q10*a*expM1(v - b, c)
	}
}

FUNCTION expM1(x,y) {
	if (fabs(x/y) < 1e-6) {
		expM1 = y*(1 - x/y/2)
	}else{
		expM1 = x/(exp(x/y) - 1)
	}
}

PROCEDURE mhnp(v) {LOCAL a, b :rest = -70
	TABLE inf, tau DEPEND celsius FROM -100 TO 100 WITH 200
	FROM i=0 TO 3 {
		a = alp(v,i)  b=bet(v,i)
		tau[i] = 1/(a + b)
		inf[i] = a/(a + b)
	}
}
UNITSON
