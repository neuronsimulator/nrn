: The idea is that the voltage follows the change in e because of high
: conductance. So every negexp event causes voltage to pass threshold

NEURON {
  SUFFIX noisychan
  NONSPECIFIC_CURRENT i
  RANGE tau, invl
  RANDOM ran
}

UNITS {
  (mA) = (milliamp)
  (mV) = (millivolt)
  (S) = (siemens)
}

PARAMETER {
  g = .1 (S/cm2)
  tau = .1 (ms)
  invl = 1 (ms)
  emax = 50 (mV)
}

ASSIGNED {
  v (mV)
  i (mA/cm2)
  tnext (ms)
}

STATE {
  e (mV)
}

INITIAL {
  random_setseq(ran, 0)
  e = -65(mV)
  tnext = negexp()
}

BEFORE STEP {
  if (t > tnext) {
    tnext = tnext + negexp()
    e = emax
  }
}

BREAKPOINT {
  SOLVE conduct METHOD cnexp
  i = g*(v - e)
}

DERIVATIVE conduct {
  e' = (-65(mV) - e)/tau
}

FUNCTION negexp()(ms) {
  negexp = invl*random_negexp(ran)
}

