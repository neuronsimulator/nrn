: Calcium channels taken from Dayan & Abbot and modified with shifts. Includes P-type persistent calcium channel. T-tpe transient calcium channel. Potassium and voltage dependent channel. Solely Calcium dependent channel.

NEURON	{
	SUFFIX cc
	USEION ca READ eca,cai WRITE ica
	USEION k READ ek WRITE ik
	RANGE gcapbar, gcap 	: P type
	RANGE gcatbar, gcat     : T type
	RANGE gkvcbar,gkvc
	RANGE gkcbar, gkc
	RANGE captempfactor, cattempfactor
}

UNITS	{
	(S) = (siemens)
	(mV) = (millivolt)
	(mA) = (milliamp)
	(molar) = (1/liter)
	(mM) = (millimolar)
	FARADAY = (faraday) (kilocoulombs)
	R = (k-mole) (joule/degC)
}

PARAMETER	{
: P-type part	
:	gcapbar = 0.0001 (S/cm2)
	gcapbar = 0 (S/cm2)
	mpcalinfhalfshift = 0
	mpcalinfslopechange = 0
	hpcalinfhalfshift = 0
	hpcalinfslopechange = 0
	captempfactor = 1
: T-type part
:	gcatbar = 0.0001 (S/cm2)
	gcatbar = 0 (S/cm2)
	mtcalinfhalfshift = 0
	mtcalinfslopechange = 0
	htcalinfhalfshift = 0
	htcalinfslopechange = 0
	cattempfactor = 1
: Voltage and Calcium dependent pottasium part
:	gkvcbar = 0.01 (S/cm2)
	gkvcbar = 0 (S/cm2)
	okvcvhalfshift = 0
	okvcslopechange = 0
	okvctempfactor = 1
	cal_base = 0.000003
: Solely calcium dependent potassium part
:	gkcbar = 0.01 (S/cm2)
	gkcbar = 0 (S/cm2)
	b = 2.5
}

ASSIGNED	{
	v	(mV)
	cai	(mM) : typically 0.001
	celsius	(degC) : typically 20
	eca	(mV)
	ica	(mA/cm2)
	gcap	(S/cm2)
	gcat	(S/cm2)
	gkvc	(S/cm2)
	gkc	(S/cm2)
	ek	(mV)
	ik	(mA/cm2)
	mpcalinf
	hpcalinf
	mpcaltau
	hpcaltau
	mtcalinf
	htcalinf
	mtcaltau
	htcaltau
}

STATE	{
	mpcal
	hpcal
	mtcal
	htcal
	okvc
	okc
}

BREAKPOINT	{
	SOLVE states METHOD cnexp
	gcap = gcapbar*mpcal*mpcal*hpcal
	gcat = gcatbar*mtcal*mtcal*htcal
	gkvc = gkvcbar*(okvc^4)
	gkc = gkcbar*okc*okc
	ica = (gcap + gcat)*(v-eca)
	ik = (gkvc+gkc)*(v-ek)
}

INITIAL	{
	settables(v)
	mpcal = mpcalinf
	hpcal = hpcalinf
	mtcal = mtcalinf
	htcal = htcalinf
	okvc = okvcinf(v)
	okc = okcinf(v)
}

DERIVATIVE states	{
    LOCAL inf, tau
    settables(v)
    mpcal' = ((mpcalinf-mpcal)/mpcaltau)
    hpcal' = ((hpcalinf-hpcal)/hpcaltau)
    mtcal' = ((mtcalinf-mtcal)/mtcaltau)
    htcal' = ((htcalinf-htcal)/htcaltau)
    inf = okvcinf(v)  tau = okvctau(v)
    okvc' = (inf-okvc)/tau
    inf = okcinf(v)  tau = okctau(v)
    okc' = (inf-okc)/tau
}

PROCEDURE settables(v (mV)) {
    :TABLE mpcalinf, hpcalinf, mpcaltau, hpcaltau, mtcalinf, htcalinf, mtcaltau, htcaltau
    :	FROM -100 TO 100 WITH 200
	mpcalinf = 1/(1+exp(-((v+57+mpcalinfhalfshift)/(6.2+mpcalinfslopechange))))
	hpcalinf = 1/(1+exp(-((v+81+hpcalinfhalfshift)/(4+hpcalinfslopechange))))
	mpcaltau = (0.612+(1/(exp(-(v+132)/16.7)+exp((v+16.8)/18.2))))*captempfactor
	if (v < -80)	{
		hpcaltau = (exp((v+467)/66.6))*captempfactor
	}else{
		hpcaltau = (28+exp(-(v+22)/10.5))*captempfactor
	}
	mtcalinf = 1/(1+exp(-((v+57+mtcalinfhalfshift)/(6.2+mtcalinfslopechange))))
	htcalinf = 1/(1+exp((v+81+htcalinfhalfshift)/(4+htcalinfslopechange)))
	mtcaltau = (0.612+(1/(exp(-(v+132)/16.7)+exp((v+16.8)/18.2))))*cattempfactor
	if (v < -80)	{
		htcaltau = (exp((v+467)/66.6))*cattempfactor
	}else{
		htcaltau = (28+exp(-(v+22)/10.5))*cattempfactor
	}
}

: Calcium and voltage dependent Potassium part

FUNCTION okvcinf(Vm (mV))	{
	UNITSOFF
	okvcinf = (cai/(cai+cal_base))*1/(1+exp(-((Vm+28.3+okvcvhalfshift)/(12.6 + okvcslopechange))))
	UNITSON
}

FUNCTION okvctau(Vm (mV)) (ms)	{
	UNITSOFF
	okvctau = (90.3-(75.1/(1+exp(-(Vm+46)/22.7))))*okvctempfactor
	UNITSON
}

: Solely Calcium dependent Potassium part

FUNCTION okcinf(Vm (mV))	{
	LOCAL a
	UNITSOFF
	a = 1.25*(10^8)*(cai)*(cai)
	okcinf = a/(a+b)
	UNITSON
}

FUNCTION okctau(Vm (mV)) (ms)	{
	LOCAL a
	UNITSOFF
	a = 1.25*(10^8)*(cai)*(cai)
	okctau=1000/(a+b)
	UNITSON
}
