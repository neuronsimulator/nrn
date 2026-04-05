TITLE hh.mod   squid sodium, potassium, and leak channels
 
COMMENT
copy of hhkin.mod and change SOLVE method to matexp
ENDCOMMENT

COMMENT
 This is the original Hodgkin-Huxley treatment for the set of sodium, 
  potassium, and leakage channels found in the squid giant axon membrane.
  ("A quantitative description of membrane current and its application 
  conduction and excitation in nerve" J.Physiol. (Lond.) 117:500-544 (1952).)
 Membrane voltage is in absolute mV and has been reversed in polarity
  from the original HH convention and shifted to reflect a resting potential
  of -65 mV.
 Remember to set a squid-appropriate temperature
 (e.g. in HOC: "celsius=6.3" or in Python: "h.celsius=6.3").
 See squid.hoc for an example of a simulation using this model.
 SW Jaslove  6 March, 1992
ENDCOMMENT
 
UNITS {
    (mA) = (milliamp)
    (mV) = (millivolt)
    (S) = (siemens)
}

? interface
NEURON {
    SUFFIX hhmatexp
    REPRESENTS NCIT:C17145   : sodium channel
    REPRESENTS NCIT:C17008   : potassium channel
    USEION na READ ena WRITE ina REPRESENTS CHEBI:29101
    USEION k READ ek WRITE ik REPRESENTS CHEBI:29103
    NONSPECIFIC_CURRENT il
    RANGE gnabar, gkbar, gl, el, gna, gk
    : `GLOBAL minf` will be replaced with `RANGE minf` if CoreNEURON enabled
    GLOBAL minf, hinf, ninf, mtau, htau, ntau
    THREADSAFE : assigned GLOBALs will be per thread
}
 
PARAMETER {
    gnabar = .12 (S/cm2)    <0,1e9>
    gkbar = .036 (S/cm2)    <0,1e9>
    gl = .0003 (S/cm2)    <0,1e9>
    el = -54.3 (mV)
}
 
STATE {
    m0h0 m1h0 m2h0 m3h0
    m0h1 m1h1 m2h1 m3h1    : m3h1 is the open state
    n0 n1 n2 n3 n4         : n4 is the open state
}
 
ASSIGNED {
    v (mV)
    celsius (degC)
    ena (mV)
    ek (mV)

    gna (S/cm2)
    gk (S/cm2)
    ina (mA/cm2)
    ik (mA/cm2)
    il (mA/cm2)
    minf hinf ninf
    mtau (ms) htau (ms) ntau (ms)
}
 
? currents
BREAKPOINT {
    SOLVE states METHOD matexp
    gna = gnabar*m3h1
    ina = gna*(v - ena)
    gk = gkbar*n4
    ik = gk*(v - ek)      
    il = gl*(v - el)
}
 
 
INITIAL {
    rates(v)
    SOLVE states STEADYSTATE sparse
}

? states
KINETIC states {  
    LOCAL am, bm, ah, bh, an, bn
    rates(v)
    am = minf/mtau
    ah = hinf/htau
    an = ninf/ntau
    bm = (1 - minf)/mtau
    bh = (1 - hinf)/htau
    bn = (1 - ninf)/ntau
    : sodium gate
    ~ m0h0 <-> m1h0 (3*am, 1*bm)
    ~ m1h0 <-> m2h0 (2*am, 2*bm)
    ~ m2h0 <-> m3h0 (1*am, 3*bm)
    ~ m0h1 <-> m1h1 (3*am, 1*bm)
    ~ m1h1 <-> m2h1 (2*am, 2*bm)
    ~ m2h1 <-> m3h1 (1*am, 3*bm)
    ~ m0h0 <-> m0h1 (1*ah, 1*bh)
    ~ m1h0 <-> m1h1 (1*ah, 1*bh)
    ~ m2h0 <-> m2h1 (1*ah, 1*bh)
    ~ m3h0 <-> m3h1 (1*ah, 1*bh)
    CONSERVE m0h0+m1h0+m2h0+m3h0+m0h1+m1h1+m2h1+m3h1 = 1

    : potassium gate
    ~ n0 <-> n1 (4*an, 1*bn)
    ~ n1 <-> n2 (3*an, 2*bn)
    ~ n2 <-> n3 (2*an, 3*bn)
    ~ n3 <-> n4 (1*an, 4*bn)
    CONSERVE n0 + n1 + n2 + n3 + n4 = 1
}
 
:LOCAL q10


? rates
PROCEDURE rates(v(mV)) {  :Computes rate and other constants at current v.
    :Call once from HOC to initialize inf at resting v.
    LOCAL  alpha, beta, sum, q10
    : `TABLE minf` will be replaced with `:TABLE minf` if CoreNEURON enabled)
    TABLE minf, mtau, hinf, htau, ninf, ntau DEPEND celsius FROM -100 TO 100 WITH 200

UNITSOFF
    q10 = 3^((celsius - 6.3)/10)
    :"m" sodium activation system
    alpha = .1 * vtrap(-(v+40),10)
    beta =  4 * exp(-(v+65)/18)
    sum = alpha + beta
    mtau = 1/(q10*sum)
    minf = alpha/sum
    :"h" sodium inactivation system
    alpha = .07 * exp(-(v+65)/20)
    beta = 1 / (exp(-(v+35)/10) + 1)
    sum = alpha + beta
    htau = 1/(q10*sum)
    hinf = alpha/sum
    :"n" potassium activation system
    alpha = .01*vtrap(-(v+55),10) 
    beta = .125*exp(-(v+65)/80)
    sum = alpha + beta
    ntau = 1/(q10*sum)
    ninf = alpha/sum
}
 
FUNCTION vtrap(x,y) {  :Traps for 0 in denominator of rate eqns.
    if (fabs(x/y) < 1e-6) {
        vtrap = y*(1 - x/y/2)
    }else{
        vtrap = x/(exp(x/y) - 1)
    }
}
 
UNITSON
