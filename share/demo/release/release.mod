TITLE transmitter release
: taken from jwm simulation

NEURON {
	SUFFIX trel
	USEION ca READ cai
}

UNITS {
	(mA) = (milliamp)
	(mV) = (millivolt)
	(mM) = (milli/liter)
}

PARAMETER {

	GenVes = 5	(mM)	<0,1e9>
	tauGen = 0	(ms)	<1e-6,1e9>
	power = 2		<0,10>
	
	al = 141	(/mM2-ms) <0,1e9>
	Kd = 1e-12	(mM2)	<0,1e9>
	Agen = 4	(/ms)	<0,1e9>
	Arev = 0	(/ms)	<0,1e9>
	Aase = 4	(/ms)	<0,1e9>
}

ASSIGNED {
	cai (mM)
}

STATE {
	Ves	(mM)
	B	(mM)
	Ach	(mM)
	X	(mM)
}

INITIAL {
	Ves = GenVes
}

BREAKPOINT {
	SOLVE release METHOD sparse
}

ASSIGNED {
	b (/ms)
	kt (/ms)
	kpow (/ms)
}

KINETIC release {
	b = Kd*al
	if (tauGen == 0) {
		kt=1e9
	}else{
		kt = 1/tauGen
	}
	kpow = al*cai^power	: set up so dimensionally correct when power=2

	~ GenVes <-> Ves	(kt, kt)
	~ Ves <-> B		(kpow, b)
	~ B <-> Ach		(Agen, Arev)
	~ Ach <-> X		(Aase, 0)
}
