TITLE Moore-Cox sodium channel
: Biophy. J. (1976) 16:171
: This paper mapped HH VClamp currents, no attention paid to resting currents. 
: Used V-jump @ T=0 to check match with HH action  potential; fig shows equal V-threshold levels
: Ramon first noted instability at rest, spontaneous impulse generation.
: Same problem noted with resimulation with NEURON. Now thresholds rather different
: Revised July 28, 1995 to remove instability. Added back reaction rate coefficients for HH beta m
: First use of  NEURON's new "Run Fitter" to find best values of these coefficients,
: using delay and fitting  ina to just beyond  peak.
: Excellent fit to HH AP with these coefficients except for "gratituitous hump" in HH
: Changed from HH reference potential level at rest to NEURON @-65mV

NEURON {
     SUFFIX MCna
     USEION na READ ena WRITE ina
     RANGE gnabar, gna, ina, lp, ml, nm, porate     
                 :lp, ml, nm, porate coefficients of HH betas for re-fitting
	GLOBAL cnt1, cnt2
}

UNITS {
     (mA) = (milliamp)
     (mV) = (millivolt)
}

PARAMETER {
     gnabar=.120 (mho/cm2)	<0,1e9>
                lp=1.9		<0, 1e9>
                ml=.75		<0, 1e9>
                nm=.3		<0, 1e9>
:                porate=1
}
STATE {
     P L M N O
}
ASSIGNED {
     v (mV)
     celsius (degC) : 6.3 
     ena (mV)
     cnt1 cnt2
     ina (mA/cm2)
     gna (mho/cm2)
}

ASSIGNED {  am (/ms)   bm (/ms)  ah (/ms)  bh (/ms)}

INITIAL {
	cnt1 = 0
	cnt2 = 0
     P=1
     rate(v*1(/mV))
     SOLVE states STEADYSTATE sparse
}

BREAKPOINT {
     SOLVE states METHOD sparse
     ina = gnabar*N*(v - ena)
	PROTECT cnt1 = cnt1 + 1
}

KINETIC states {
	PROTECT cnt2 = cnt2 + 1
     rate(v*1(/mV))
:     CONSERVE P + L + M + N + O = 1
     ~ P <-> L (am, lp*bm)    :back reaction in original = 3.5   
     ~ L <-> M (2*am, ml*bm)          :back reaction in original = 0
     ~ M <-> N (3*am, nm*bm)         :back reaction in original = 0
     ~ N <-> O (1.1*bh, 0)
     ~ N <-> P (3*bm, 0)
     ~ P <-> O (bh, ah)       :back reaction in original = 1,
  			      : found this to still be good
}

UNITSOFF
FUNCTION alp(v(mV),i) { LOCAL a,b,c,q10 :rest = -65  order m,h
     v = -v - 65  :convert to hh convention
     q10 = 3^((celsius - 6.3)/10)
     if (i==0) {
          alp = q10*.1*expM1(v + 25, 10)
     }else if (i==1){
          alp = q10*.07*exp(v/20)
     }
}

FUNCTION bet(v,i) { LOCAL a,b,c,q10 :rest = -70  order m,h
     v = -v - 65
     q10 = 3^((celsius - 6.3)/10)
     if (i==0) {
          bet = q10* 4*exp(v/18)
     }else if (i==1){
          bet = q10*1/(exp(.1*v + 3) + 1)
     }
}

FUNCTION expM1(x,y) {
     if (fabs(x/y) < 1e-6) {
          expM1 = y*(1 - x/y/2) : for singular point
     }else{
          expM1 = x/(exp(x/y) - 1)
     }
}

PROCEDURE rate(v) {LOCAL a, b, tau :rest = -65
     TABLE am, ah, bm, bh DEPEND celsius FROM -100 TO 100 WITH 200
     am = alp(v, 0)
     ah = alp(v, 1)
     bm = bet(v, 0)
     bh = bet(v, 1)
}
UNITSON






