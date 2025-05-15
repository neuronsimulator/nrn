: Modified from https://senselab.med.yale.edu/modeldb/ShowModel.asp?model=150551&file=\AshhadNarayanan2013\mglur.mod
: Authors: Ashhad S and Narayanan R,  2013 

: this mod file was significantly altered - there should not be any
: ip3i (transmembrane current for ip3) since ip3 is generated within the membrane
: ip3 degradation is now handled by rxd
: units need adjustment

INDEPENDENT {t FROM 0 TO 1 WITH 1 (ms)}

NEURON {
  POINT_PROCESS mGLUR
  RANGE G, C, lastrelease
  RANGE Cmax, Cdur, Deadtime, K1, K2, initmGluR
  RANGE rip3 : ip3 production rate
  RANGE ip3 : mod-file internal ip3 produced, just need it for fluxes for ip3 production
  RANGE scalef : scaling factor to correct units
  RANGE degGluRate
  : USEION ip3 READ ip3i VALENCE 0 : only needed if want to see rxd's ip3 -- not required
}

UNITS {
  (mM) = (milli/liter)
  (uM)=  (micro/liter)
  (mA)    = (milliamp) 
}

PARAMETER {	
  initmGluR=0.3e-3 (mM): Bhalla & Iyenger Science  1999
  Cmax	= 1	(mM)		: max transmitter concentration
  Deadtime = 1	(ms)		: mimimum time between release events
  K1	= 0.28	(/ms uM)	: forward binding rate to receptor from Bhalla et al
  K2	= 0.016 (/ms)		: backward (unbinding) rate of receptor from Bhalla et al
  K_PLC = 5 (uM)		:total concentration of PLC
  K_PIP2 = 160 (uM)		:total concentration of PIP2
  K_G=25 (uM)
  :kplc and Vmax describe aPLC catalyzing IP3 production from PIP2
  kfplc = 0.83(/ms)
  kbplc = 0.68 (/ms) : 0.1/ms in the paper; added to Vmax1=0.58/ms in the paper
  Vmax1 = 0.58 (/ms)
  :D5 and D6 describe Glu_mGluR catalyzing G_alpha production, Km2=(D6f+D5B)/D5f
  D5f = 15 (/ms)
  D5b = 7.2 (/ms)
  D6f = 1.8(/ms)
  Vmax2 = 1.8 (/ms)
  Km2 = 0.6 (uM)
  :G2 describe aG binding to PLC
  G2f = 100 (/ms)
  G2b = 100 (/ms)
  :degradation of aG (D7f) and IP3 (G9f)
  D7f =9  (/ms)
  G9f = 0.75(/ms)  :4 in original paper
  Cdur=2 (ms)			: transmitter duration (rising phase)
  scalef = 1e10
  degGluRate = 1.0 (/ms) : should lookup value
}

ASSIGNED {
  C		(mM)		: transmitter concentration
  lastrelease	(ms)		: time of last spike
  rip3
}

STATE {
  aG				: fraction of activated G-protein
  aPLC_aG
  aPLC_PIP2
  Glu : glutamate
  degGlu : degraded glutamate (an unused garbage pool)
  Glu_mGluR
  GG_mGluR
  ip3
  : degip3
  mGluR
  PLC
  PIP2
  G
  ip3i
}

INITIAL {
  Glu = 0
  degGlu = 0
  Glu_mGluR = 0
  GG_mGluR = 0
  aPLC_aG=0 :0.0035 
  aPLC_PIP2=0
  aG =0: 0.0007
  ip3= 0
  G=K_G-(aG+GG_mGluR+aPLC_aG+aPLC_PIP2)
  PLC=K_PLC-(aPLC_aG+aPLC_PIP2)
  PIP2=K_PIP2-(aPLC_PIP2+ip3)
  mGluR=initmGluR
  lastrelease = -1e8
  rip3 = 0
  : degip3 = 0
}

BREAKPOINT {
  : evaluateC() : original
  SOLVE bindkin METHOD sparse
}

KINETIC bindkin { LOCAL a,b

  : ~ mGluR <-> Glu_mGluR (C*K1, K2) : original

  ~ Glu+mGluR <-> Glu_mGluR (K1, K2)
  ~ Glu <-> degGlu (degGluRate,0)

  ~ Glu_mGluR + G <-> GG_mGluR (D5f,D5b)
  ~ GG_mGluR <-> aG+mGluR (D6f,0)
  ~ aG <-> G (D7f,0)
  ~ aG+PLC <-> aPLC_aG (G2f, G2b)
  ~ aPLC_aG+PIP2 <-> aPLC_PIP2 (kfplc,kbplc)
  ~ aPLC_PIP2 <-> ip3 (Vmax1,0)
  rip3 = f_flux * scalef : only using this mod file for ip3 production, degradation done in rxd!
  : a = f_flux
  : ~ ip3 <-> degip3 (G9f,0)
  : b = f_flux
  : rip3 = (a - b) * scalef : scaled production rate
  :CONSERVE G+aG+GG_mGluR+aPLC_aG+aPLC_PIP2=K_G
  :CONSERVE PLC+aPLC_aG+aPLC_PIP2=K_PLC
  :CONSERVE PIP2+aPLC_PIP2+ip3=K_PIP2
}

PROCEDURE evaluateC() {
  LOCAL q
  q = ((t - lastrelease) - Cdur)		: time since last release ended
  if (q >= 0 && q <= Deadtime && C == Cmax) {	: in dead time after release
    C = 0.
  }
}

NET_RECEIVE (weight)  { 
  Glu = Glu + weight : this safe ?
: 
:  LOCAL q
:  q = ((t - lastrelease) - Cdur)		: time since last release ended
:  : Spike has arrived, ready for another release?
:  if (q > Deadtime) {
:    C = Cmax			: start new release
:    : shouldn't C increase by weight rather than Cmax so that NetCon weights have an effect??
:    lastrelease = t
:  } 
}






