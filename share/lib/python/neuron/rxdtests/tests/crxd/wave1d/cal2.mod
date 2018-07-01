TITLE L-calcium channel
: L-type calcium channel with [Ca]i inactivation
: from Jaffe, D. B., Ross, W. N., Lisman, J. E., Laser-Ross, N., Miyakawa, H., and Johnston, D. A. A model for dendritic Ca2
: accumulation in hippocampal pyramidal neurons based on fluorescence imaging measurements. J. Neurophysiol. 71:1O65-1077 1994.
: conduction density estimate of 50-200 pS/mu2; 0.0025 S/cm2 (5-20 channels of 10 each)
: M. Migliore, E. Cook, D.B. Jaffe, D.A. Turner and D. Johnston, Computer simulations of morphologically reconstructed CA3
: hippocampal neurons, J. Neurophysiol. 73, 1157-1168 (1995). 
: adapted from http://senselab.med.yale.edu/modeldb/ShowModel.asp?model=3263&file=\ca3_db\cal2.mod
: this version from https://senselab.med.yale.edu/ModelDB/ShowModel.asp?model=148094&file=\kv72-R213QW-mutations\cal2.mod
: Miceli F, Soldovieri MV, Ambrosino P, Barrese V, Migliore M, Cilio MR, Taglialatela M (2013) Genotype-phenotype
: correlations in neonatal epilepsies caused by mutations in the voltage sensor of Kv7.2 potassium channel subunits. PNAS 110:4386-4391

UNITS {
  (mA) = (milliamp)
  (mV) = (millivolt)

  FARADAY = 96520 (coul)
  R = 8.3134 (joule/degC)
  KTOMV = .0853 (mV/degC)
}

PARAMETER {
  v (mV)
  celsius 	(degC)
  gcalbar=.003 (mho/cm2)
  ki=.001 (mM)
  cai = 50.e-6 (mM)
  cao = 2 (mM)
  q10 = 5
  mmin=0.2
  tfa = 1
  a0m =0.1
  zetam = 2
  vhalfm = 4
  gmm=0.1	
  ggk
}


NEURON {
  SUFFIX cal
  USEION ca READ cai,cao WRITE ica
  RANGE gcalbar,cai, ica, gcal, ggk
  GLOBAL minf,tau
}

STATE {
  m
}

ASSIGNED {
  ica (mA/cm2)
  gcal (mho/cm2)
  minf
  tau   (ms)
}

INITIAL {
  rate(v)
  m = minf
  gcal = gcalbar*m*m*h2(cai)
  ggk=ghk(v,cai,cao)
  ica = gcal*ggk
}

BREAKPOINT {
  SOLVE state METHOD cnexp
  gcal = gcalbar*m*m*h2(cai)
  ggk=ghk(v,cai,cao)
  ica = gcal*ggk
}

FUNCTION h2(cai(mM)) {
  h2 = ki/(ki+cai)
}


FUNCTION ghk(v(mV), ci(mM), co(mM)) (mV) {
  LOCAL nu,f
  f = KTF(celsius)/2
  nu = v/f
  ghk=-f*(1. - (ci/co)*exp(nu))*efun(nu)
}

FUNCTION KTF(celsius (DegC)) (mV) {
  KTF = ((25./293.15)*(celsius + 273.15))
}


FUNCTION efun(z) {
  if (fabs(z) < 1e-4) {
    efun = 1 - z/2
  }else{
    efun = z/(exp(z) - 1)
  }
}

FUNCTION alp(v(mV)) (1/ms) {
  alp = 15.69*(-1.0*v+81.5)/(exp((-1.0*v+81.5)/10.0)-1.0)
}

FUNCTION bet(v(mV)) (1/ms) {
  bet = 0.29*exp(-v/10.86)
}

FUNCTION alpmt(v(mV)) {
  alpmt = exp(0.0378*zetam*(v-vhalfm)) 
}

FUNCTION betmt(v(mV)) {
  betmt = exp(0.0378*zetam*gmm*(v-vhalfm)) 
}

DERIVATIVE state {  
  rate(v)
  m' = (minf - m)/tau
}

PROCEDURE rate(v (mV)) { :callable from hoc
  LOCAL a, b, qt
  qt=q10^((celsius-25)/10)
  a = alp(v)
  b = 1/((a + bet(v)))
  minf = a*b
  tau = betmt(v)/(qt*a0m*(1+alpmt(v)))
  if (tau<mmin/qt) {tau=mmin/qt}
}
