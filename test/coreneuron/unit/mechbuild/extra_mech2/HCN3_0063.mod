:[$URL: https://bbpteam.epfl.ch/svn/analysis/trunk/IonChannel/xmlTomod/CreateMOD.c $]
:[$Revision: 1366 $]
:[$Date: 2010-03-26 10:23:02 +0100 (Fri, 26 Mar 2010) $]
:[$Author: rajnish $]
:Comment :
:Reference :Cellular expression and functional characterization of four hyperpolarization-activated pacemaker channels in cardiac and neuronal tissues. Eur. J. Biochem., 2001,268,1646-52

NEURON	{
	SUFFIX HCN3_0063
	NONSPECIFIC_CURRENT ihcn
	RANGE gHCN3bar, gHCN3, ihcn, BBiD 
}

UNITS	{
	(S) = (siemens)
	(mV) = (millivolt)
	(mA) = (milliamp)
}

PARAMETER	{
	gHCN3bar = 0.00001 (S/cm2) 
	BBiD = 63 
	ehcn = -45.0 (mV)
}

ASSIGNED	{
	v	(mV)
	ihcn	(mA/cm2)
	gHCN3	(S/cm2)
	mInf
	mTau
}

STATE	{ 
	m
}

BREAKPOINT	{
	SOLVE states METHOD cnexp
	gHCN3 = gHCN3bar*m
	ihcn = gHCN3*(v-ehcn)
}

DERIVATIVE states	{
	rates()
	m' = (mInf-m)/mTau
}

INITIAL{
	rates()
	m = mInf
}

PROCEDURE rates(){
	UNITSOFF 
		mInf = 1.0000/(1+exp((v- -96)/8.6)) 
		mTau = 265.0000
	UNITSON
}
