:Comment : The persistent component of the K current
:Reference : :		Voltage-gated K+ channels in layer 5 neocortical pyramidal neurones from young rats:subtypes and gradients,Korngreen and Sakmann, J. Physiology, 2000
:Comment : shifted -10 mv to correct for junction potential
:Comment: corrected rates using q10 = 2.3, target temperature 34, orginal 21


NEURON	{
	SUFFIX K_Pst
	USEION k READ ek WRITE ik
	RANGE gK_Pstbar, gK_Pst, ik
}

UNITS	{
	(S) = (siemens)
	(mV) = (millivolt)
	(mA) = (milliamp)
}

PARAMETER	{
	gK_Pstbar = 0.00001 (S/cm2)
}

ASSIGNED	{
	v	(mV)
	ek	(mV)
	ik	(mA/cm2)
	gK_Pst	(S/cm2)
	mInf
	mTau
	hInf
	hTau
}

STATE	{
	m
	h
}

BREAKPOINT	{
	SOLVE states METHOD cnexp
	gK_Pst = gK_Pstbar*m*m*h
	ik = gK_Pst*(v-ek)
}

DERIVATIVE states	{
	rates()
	m' = (mInf-m)/mTau
	h' = (hInf-h)/hTau
}

INITIAL{
	rates()
	m = mInf
	h = hInf
}

PROCEDURE rates(){
  LOCAL qt
  qt = 2.3^((34-21)/10)
	UNITSOFF
		v = v + 10
		mInf =  (1/(1 + exp(-(v+1)/12)))
        if(v<-50){
		    mTau =  (1.25+175.03*exp(-v * -0.026))/qt
        }else{
            mTau = ((1.25+13*exp(-v*0.026)))/qt
        }
		hInf =  1/(1 + exp(-(v+54)/-11))
		hTau =  (360+(1010+24*(v+55))*exp(-((v+75)/48)^2))/qt
		v = v - 10
	UNITSON
}
